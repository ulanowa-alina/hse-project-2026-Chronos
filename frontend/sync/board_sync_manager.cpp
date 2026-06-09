#include "board_sync_manager.hpp"

#include "sync_json_helpers.hpp"

#include <QJsonDocument>
#include <QUrl>

BoardSyncManager::BoardSyncManager(QSqlDatabase& db, NetworkManager* network_manager)
    : db_(db)
    , network_manager_(network_manager) {
}

QString BoardSyncManager::modelName() const {
    return "board";
}

QString BoardSyncManager::loadEndpoint() const {
    return network_manager_->boards_get_all_url_;
}

bool BoardSyncManager::handlesLoadEndpoint(const QString& endpoint) const {
    return endpoint == network_manager_->boards_get_all_url_;
}

LocalBoard BoardSyncManager::boardFromJson(const QJsonObject& obj) const {
    const QString created_at = jsonTimestamp(obj, "created_at");
    const QString updated_at = jsonTimestamp(obj, "updated_at", "created_at");
    return LocalBoard(obj["id"].toInt(), obj["user_id"].toInt(), obj["title"].toString(),
                      obj["description"].toString(), created_at, updated_at, QString(), SyncStatus::SYNCED, 1);
}

void BoardSyncManager::saveFromServer(const LocalBoard& board) {
    LocalBoardRepository repo(db_);
    const auto existing = repo.findById(board.id_);
    if (existing && existing->sync_status_ == SyncStatus::PENDING &&
        existing->updated_at_ > board.updated_at_) {
        LocalBoard conflict = *existing;
        conflict.sync_status_ = SyncStatus::CONFLICT;
        repo.save(conflict);
        return;
    }
    repo.save(board);
}

void BoardSyncManager::parse(const QJsonArray& data) {
    for (const auto& value : data) {
        if (!value.isObject()) {
            continue;
        }
        saveFromServer(boardFromJson(value.toObject()));
    }
}

void BoardSyncManager::parseObject(const QJsonObject& data) {
    saveFromServer(boardFromJson(data));
}

void BoardSyncManager::load() {
    network_manager_->GET(network_manager_->boards_get_all_url_);
}

void BoardSyncManager::sync() {
    LocalBoardRepository repo(db_);
    const std::vector<LocalBoard> pending = repo.findUnsynced();

    for (const auto& board : pending) {
        if (board.title_.trimmed().isEmpty() && board.deleted_at_.isEmpty()) {
            continue;
        }

        if (!board.deleted_at_.isEmpty()) {
            QJsonObject json;
            json["board_id"] = board.id_;
            network_manager_->syncDELETE(network_manager_->boards_delete_url_, json, modelName(),
                                         board.id_, "delete");
            continue;
        }

        if (board.server_version_ == 0) {
            QJsonObject json;
            json["user_id"] = board.user_id_;
            json["title"] = board.title_;
            json["description"] = board.description_;
            network_manager_->syncPOST(network_manager_->boards_create_url_, json, modelName(),
                                       board.id_, "create");
        } else {
            QJsonObject json;
            json["board_id"] = board.id_;
            json["title"] = board.title_;
            json["description"] = board.description_;
            network_manager_->syncPATCH(network_manager_->boards_edit_url_, json, modelName(),
                                        board.id_, "update");
        }
    }
}

bool BoardSyncManager::handlePushResponse(const QString& endpoint, const QByteArray& data, int code,
                                          int local_id, const QString& operation) {
    LocalBoardRepository repo(db_);

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
        saveFromServer(boardFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    if (operation == "update") {
        const QJsonObject obj = QJsonDocument::fromJson(data).object()["data"].toObject();
        saveFromServer(boardFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    return false;
}
