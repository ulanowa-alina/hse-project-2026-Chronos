#include "sync_manager.h"

#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_status_repository.hpp"
#include "../local_repositories/local_task_repository.hpp"

#include <QJsonObject>
#include <QObject>

SyncManager::SyncManager(QSqlDatabase& db, NetworkManager* manager)
    : db_(db)
    , network_manager_(manager) {
}
void SyncManager::syncBoards() {
    LocalBoardRepository repo(db_);

    std::vector<LocalBoard> should_synced = repo.findUnsynced();

    for (auto& board : should_synced) {
        if (board.is_deleted_ == 1) {
            QJsonObject json;
            json["board_id"] = board.id_;

            network_manager_->DELETE(network_manager_->boards_delete_url_, json);
            continue;
        }

        if (board.is_new_) {
            QJsonObject json;
            json["title"] = board.title_;
            json["description"] = board.description_;
            json["is_private"] = board.is_private_;
            network_manager_->POST(network_manager_->boards_create_url_, json);
            board.is_new_ = 0;
        } else {
            QJsonObject json;
            json["board_id"] = board.id_;
            json["title"] = board.title_;
            json["description"] = board.description_;
            json["is_private"] = board.is_private_;
            network_manager_->PATCH(network_manager_->boards_edit_url_, json);
        }
        repo.markSynced(board.id_);
    }
}

void SyncManager::syncStatuses() {
    LocalSatusRepository repo(db_);

    std::vector<LocalStatus> should_synced = repo.findUnsynced();

    for (auto& status : should_synced) {
        if (status.is_deleted_ == 1) {
            QJsonObject json;
            json["status_id"] = status.id_;

            network_manager_->DELETE(network_manager_->statuses_delete_url_, json);
            continue;
        }

        if (status.is_new_) {
            QJsonObject json;
            json["board_id"] = status.board_id_;
            json["name"] = status.name_;
            json["position"] = status.position_;
            network_manager_->POST(network_manager_->statuses_create_url_, json);
            status.is_new_ = 0;
        } else {
            QJsonObject json;
            json["status_id"] = status.id_;
            json["name"] = status.name_;
            json["position"] = status.position_;
            network_manager_->PATCH(network_manager_->statuses_edit_url_, json);
        }
        repo.markSynced(status.id_);
    }
}

void SyncManager::syncTasks() {
    LocalTaskRepository repo(db_);

    std::vector<LocalTask> should_synced = repo.findUnsynced();

    for (auto& task : should_synced) {
        if (task.is_deleted_ == 1) {
            QJsonObject json;
            json["task_id"] = task.id_;

            network_manager_->DELETE(network_manager_->tasks_delete_url_, json);
            continue;
        }

        if (task.is_new_) {
            QJsonObject json;
            json["board_id"] = task.board_id_;
            json["title"] = task.title_;
            json["description"] = task.description_;
            json["status_id"] = task.status_id_;
            json["priority_color"] = task.priority_color_;
            network_manager_->POST(network_manager_->tasks_create_url_, json);
            task.is_new_ = 0;
        } else {
            QJsonObject json;
            json["task_id"] = task.id_;
            json["title"] = task.title_;
            json["description"] = task.description_;
            json["status_id"] = task.status_id_;
            json["priority_color"] = task.priority_color_;
            network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
        }
        repo.markSynced(task.id_);
    }
}

void SyncManager::syncAll() {
    syncBoards();
    syncStatuses();
    syncTasks();
}

void SyncManager::loadBoards() {
    network_manager_->GET(network_manager_->boards_get_all_url_);
}

void SyncManager::parsingBoards(const QJsonArray& boards) {

    LocalBoardRepository repo(db_);
    for (auto& json : boards) {
        if (!json.isObject()) {
            continue;
        }

        QJsonObject board_obj = json.toObject();
        LocalBoard board(board_obj["id"].toInt(), board_obj["title"].toString(),
                         board_obj["description"].toString(), board_obj["is_private"].toInt(),
                         board_obj["created_at"].toString(), board_obj["updated_at"].toString(), 1,
                         0, 0);

        repo.save(board);
    }
}
