#include "status_sync_manager.hpp"

#include "../local_repositories/local_board_repository.hpp"

#include <QDebug>
#include <QJsonDocument>

StatusSyncManager::StatusSyncManager(QSqlDatabase& db, NetworkManager* network_manager)
    : db_(db)
    , network_manager_(network_manager) {
}

QString StatusSyncManager::modelName() const {
    return "status";
}

QString StatusSyncManager::loadEndpoint() const {
    return network_manager_->statuses_get_all_url_;
}

bool StatusSyncManager::handlesLoadEndpoint(const QString& endpoint) const {
    return endpoint.startsWith(network_manager_->statuses_get_all_url_);
}

bool StatusSyncManager::isParentBoardSynced(int board_id) const {
    LocalBoardRepository repo(db_);
    const auto board = repo.findById(board_id);
    return board.has_value() && board->sync_status_ == SyncStatus::SYNCED && board_id > 0;
}

LocalStatus StatusSyncManager::statusFromJson(const QJsonObject& obj) const {
    return LocalStatus(obj["id"].toInt(), obj["board_id"].toInt(), obj["name"].toString(),
                       obj["position"].toInt(), obj.value("created_at").toString(),
                       obj.value("updated_at").toString(), QString(), SyncStatus::SYNCED, 1);
}

void StatusSyncManager::saveFromServer(const LocalStatus& status) {
    LocalBoardRepository board_repo(db_);
    if (!board_repo.findById(status.board_id_)) {
        return;
    }

    LocalStatusRepository repo(db_);
    const auto existing = repo.findById(status.id_);
    if (existing && existing->sync_status_ == SyncStatus::PENDING &&
        existing->updated_at_ > status.updated_at_) {
        LocalStatus conflict = *existing;
        conflict.sync_status_ = SyncStatus::CONFLICT;
        repo.save(conflict);
        return;
    }

    try {
        repo.save(status);
    } catch (const std::exception& e) {
        qDebug() << "StatusSyncManager: failed to save status:" << e.what();
    }
}

void StatusSyncManager::parse(const QJsonArray& data) {
    for (const auto& value : data) {
        if (!value.isObject()) {
            continue;
        }
        saveFromServer(statusFromJson(value.toObject()));
    }
}

void StatusSyncManager::parseObject(const QJsonObject& data) {
    saveFromServer(statusFromJson(data));
}

void StatusSyncManager::load() {
    network_manager_->GET(network_manager_->statuses_get_all_url_);
}

void StatusSyncManager::sync() {
    LocalStatusRepository repo(db_);
    const std::vector<LocalStatus> pending = repo.findUnsynced();

    for (const auto& status : pending) {
        if (status.board_id_ < 0 || !isParentBoardSynced(status.board_id_)) {
            continue;
        }

        if (!status.deleted_at_.isEmpty()) {
            QJsonObject json;
            json["status_id"] = status.id_;
            network_manager_->syncDELETE(network_manager_->statuses_delete_url_, json, modelName(),
                                         status.id_, "delete");
            continue;
        }

        if (status.server_version_ == 0) {
            QJsonObject json;
            json["board_id"] = status.board_id_;
            json["name"] = status.name_;
            json["position"] = status.position_;
            network_manager_->syncPOST(network_manager_->statuses_create_url_, json, modelName(),
                                       status.id_, "create");
        } else {
            QJsonObject json;
            json["status_id"] = status.id_;
            json["name"] = status.name_;
            json["position"] = status.position_;
            network_manager_->syncPATCH(network_manager_->statuses_edit_url_, json, modelName(),
                                        status.id_, "update");
        }
    }
}

bool StatusSyncManager::handlePushResponse(const QString& endpoint, const QByteArray& data,
                                           int code, int local_id, const QString& operation) {
    LocalStatusRepository repo(db_);

    if (operation == "delete") {
        if (code == 204 || code == 200) {
            repo.deleteById(local_id);
            return true;
        }
        return false;
    }

    if (code < 200 || code >= 300) {
        return false;
    }

    if (operation == "create") {
        const QJsonObject obj = QJsonDocument::fromJson(data).object()["data"].toObject();
        const int server_id = obj["id"].toInt();
        if (local_id < 0 && server_id > 0) {
            repo.replaceId(local_id, server_id);
            local_id = server_id;
        }
        saveFromServer(statusFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    if (operation == "update") {
        const QJsonObject obj = QJsonDocument::fromJson(data).object()["data"].toObject();
        saveFromServer(statusFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    return false;
}
