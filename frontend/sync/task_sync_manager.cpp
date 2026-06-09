#include "task_sync_manager.hpp"

#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_status_repository.hpp"
#include "sync_json_helpers.hpp"

#include <QDebug>
#include <QJsonDocument>

TaskSyncManager::TaskSyncManager(QSqlDatabase& db, NetworkManager* network_manager)
    : db_(db)
    , network_manager_(network_manager) {
}

QString TaskSyncManager::modelName() const {
    return "task";
}

QString TaskSyncManager::loadEndpoint() const {
    return network_manager_->tasks_get_all_url_;
}

bool TaskSyncManager::handlesLoadEndpoint(const QString& endpoint) const {
    return endpoint.startsWith(network_manager_->tasks_get_all_url_);
}

bool TaskSyncManager::isParentBoardSynced(int board_id) const {
    LocalBoardRepository repo(db_);
    const auto board = repo.findById(board_id);
    return board.has_value() && board->sync_status_ == SyncStatus::SYNCED && board_id > 0;
}

LocalTask TaskSyncManager::taskFromJson(const QJsonObject& obj) const {
    const QString created_at = jsonTimestamp(obj, "created_at");
    const QString updated_at = jsonTimestamp(obj, "updated_at", "created_at");
    QString priority_color = obj["priority_color"].toString();
    if (priority_color.isEmpty()) {
        priority_color = QStringLiteral("gray");
    }
    const bool is_completed = obj["is_completed"].isBool() ? obj["is_completed"].toBool()
                                                            : obj["is_completed"].toInt() != 0;
    return LocalTask(obj["id"].toInt(), obj["board_id"].toInt(), obj["title"].toString(),
                     obj["status_id"].toInt(), priority_color, obj["description"].toString(),
                     obj.value("deadline").toString(), is_completed, created_at, updated_at, QString(),
                     SyncStatus::SYNCED, 1);
}

void TaskSyncManager::saveFromServer(const LocalTask& task) {
    LocalBoardRepository board_repo(db_);
    LocalStatusRepository status_repo(db_);
    if (!board_repo.findById(task.board_id_) || !status_repo.findById(task.status_id_)) {
        return;
    }

    LocalTaskRepository repo(db_);
    const auto existing = repo.findById(task.id_);
    if (existing && existing->sync_status_ == SyncStatus::PENDING &&
        existing->updated_at_ > task.updated_at_) {
        LocalTask conflict = *existing;
        conflict.sync_status_ = SyncStatus::CONFLICT;
        repo.save(conflict);
        return;
    }

    try {
        repo.save(task);
    } catch (const std::exception& e) {
        qDebug() << "TaskSyncManager: failed to save task:" << e.what();
    }
}

void TaskSyncManager::parse(const QJsonArray& data) {
    for (const auto& value : data) {
        if (!value.isObject()) {
            continue;
        }
        saveFromServer(taskFromJson(value.toObject()));
    }
}

void TaskSyncManager::parseObject(const QJsonObject& data) {
    saveFromServer(taskFromJson(data));
}

void TaskSyncManager::load() {
    network_manager_->GET(network_manager_->tasks_get_all_url_);
}

void TaskSyncManager::sync() {
    LocalTaskRepository repo(db_);
    const std::vector<LocalTask> pending = repo.findUnsynced();

    for (const auto& task : pending) {
        if (task.board_id_ < 0 || !isParentBoardSynced(task.board_id_)) {
            continue;
        }

        if (task.title_.trimmed().isEmpty() && task.deleted_at_.isEmpty()) {
            continue;
        }

        if (!task.deleted_at_.isEmpty()) {
            QJsonObject json;
            json["task_id"] = task.id_;
            network_manager_->syncDELETE(network_manager_->tasks_delete_url_, json, modelName(),
                                         task.id_, "delete");
            continue;
        }

        if (task.server_version_ == 0) {
            QJsonObject json;
            json["board_id"] = task.board_id_;
            json["title"] = task.title_;
            json["description"] = task.description_;
            json["status_id"] = task.status_id_;
            json["priority_color"] = task.priority_color_;
            json["is_completed"] = task.is_completed_;
            if (!task.deadline_.isEmpty()) {
                json["deadline"] = task.deadline_;
            }
            network_manager_->syncPOST(network_manager_->tasks_create_url_, json, modelName(),
                                       task.id_, "create");
        } else {
            QJsonObject json;
            json["task_id"] = task.id_;
            json["title"] = task.title_;
            json["description"] = task.description_;
            json["status_id"] = task.status_id_;
            json["priority_color"] = task.priority_color_;
            json["is_completed"] = task.is_completed_;
            if (!task.deadline_.isEmpty()) {
                json["deadline"] = task.deadline_;
            }
            network_manager_->syncPATCH(network_manager_->tasks_edit_url_, json, modelName(),
                                        task.id_, "update");
        }
    }
}

bool TaskSyncManager::handlePushResponse(const QString& endpoint, const QByteArray& data, int code,
                                         int local_id, const QString& operation) {
    LocalTaskRepository repo(db_);

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
        saveFromServer(taskFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    if (operation == "update") {
        const QJsonObject obj = QJsonDocument::fromJson(data).object()["data"].toObject();
        saveFromServer(taskFromJson(obj));
        repo.markSynced(local_id);
        return true;
    }

    return false;
}
