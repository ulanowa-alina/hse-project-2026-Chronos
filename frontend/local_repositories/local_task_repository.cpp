#include "local_task_repository.hpp"

#include "repository_utils.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalTask createTask(const QSqlQuery& query) {
    return LocalTask(query.value("id").toInt(), query.value("board_id").toInt(),
                     query.value("title").toString(), query.value("status_id").toInt(),
                     query.value("priority_color").toString(),
                     query.value("description").toString(), query.value("deadline").toString(),
                     query.value("is_completed").toBool(), query.value("created_at").toString(),
                     query.value("updated_at").toString(), query.value("deleted_at").toString(),
                     stringToSyncStatus(query.value("sync_status").toString()),
                     query.value("server_version").toInt());
}

LocalTaskRepository::LocalTaskRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalTask LocalTaskRepository::insert(const LocalTask& task) {
    if (task.title_.trimmed().isEmpty()) {
        throw std::runtime_error("LocalTaskRepository: title must not be empty");
    }

    const QString created_at = processingTimestamp(task.created_at_);
    const QString updated_at =
        processingTimestamp(task.updated_at_.isEmpty() ? created_at : task.updated_at_);

    QSqlQuery query(db_);

    query.prepare(
        "INSERT INTO tasks ("
        "id, board_id, title, description, status_id, priority_color, deadline, is_completed, "
        "created_at, updated_at, deleted_at, sync_status, server_version"
        ") VALUES ("
        ":id, :board_id, :title, :description, :status_id, :priority_color, :deadline, "
        ":is_completed, "
        ":created_at, :updated_at, :deleted_at, :sync_status, :server_version"
        ")");

    query.bindValue(":id", task.id_);
    query.bindValue(":board_id", task.board_id_);
    query.bindValue(":title", task.title_.trimmed());
    query.bindValue(":description", task.description_);
    query.bindValue(":status_id", task.status_id_);
    query.bindValue(":priority_color", task.priority_color_);
    query.bindValue(":deadline", task.deadline_);
    query.bindValue(":is_completed", task.is_completed_ ? 1 : 0);
    query.bindValue(":created_at", created_at);
    query.bindValue(":updated_at", updated_at);
    query.bindValue(":deleted_at", task.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : task.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(task.sync_status_));
    query.bindValue(":server_version", task.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    LocalTask saved = task;
    saved.title_ = task.title_.trimmed();
    saved.created_at_ = created_at;
    saved.updated_at_ = updated_at;
    return saved;
}

LocalTask LocalTaskRepository::update(const LocalTask& task) {
    if (task.title_.trimmed().isEmpty()) {
        throw std::runtime_error("LocalTaskRepository: title must not be empty");
    }

    const QString updated_at = processingTimestamp(task.updated_at_);

    QSqlQuery query(db_);

    query.prepare("UPDATE tasks SET "
                  "board_id = :board_id, "
                  "title = :title, "
                  "description = :description, "
                  "status_id = :status_id, "
                  "priority_color = :priority_color, "
                  "deadline = :deadline, "
                  "is_completed = :is_completed, "
                  "created_at = :created_at, "
                  "updated_at = :updated_at, "
                  "deleted_at = :deleted_at, "
                  "sync_status = :sync_status, "
                  "server_version = :server_version "
                  "WHERE id = :id");

    query.bindValue(":id", task.id_);
    query.bindValue(":board_id", task.board_id_);
    query.bindValue(":title", task.title_.trimmed());
    query.bindValue(":description", task.description_);
    query.bindValue(":status_id", task.status_id_);
    query.bindValue(":priority_color", task.priority_color_);
    query.bindValue(":deadline", task.deadline_);
    query.bindValue(":is_completed", task.is_completed_ ? 1 : 0);
    query.bindValue(":created_at", processingTimestamp(task.created_at_));
    query.bindValue(":updated_at", updated_at);
    query.bindValue(":deleted_at", task.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : task.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(task.sync_status_));
    query.bindValue(":server_version", task.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: update error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: update error: " + query.lastError().text()).toStdString());
    }

    return task;
}

LocalTask LocalTaskRepository::save(const LocalTask& task) {
    try {
        if (!findById(task.id_)) {
            return insert(task);
        }
        return update(task);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Task: ") + e.what());
    }
}

std::optional<LocalTask> LocalTaskRepository::findById(int task_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "is_completed, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM tasks WHERE id = :id");
    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error find by id: " + query.lastError().text()).toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createTask(query);
}

std::vector<LocalTask> LocalTaskRepository::findAll() {
    QSqlQuery query(db_);
    query.prepare("SELECT t.id, t.board_id, t.title, t.description, t.status_id, "
                  "t.priority_color, t.deadline, t.is_completed, "
                  "t.created_at, t.updated_at, t.deleted_at, t.sync_status, t.server_version "
                  "FROM tasks t "
                  "JOIN statuses s ON s.id = t.status_id "
                  "WHERE t.deleted_at IS NULL AND s.deleted_at IS NULL");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error find all tasks: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalTask> tasks;
    while (query.next()) {
        tasks.push_back(createTask(query));
    }

    return tasks;
}

std::vector<LocalTask> LocalTaskRepository::findByBoardId(int board_id) {

    QSqlQuery query(db_);

    query.prepare("SELECT t.id, t.board_id, t.title, t.description, t.status_id, "
                  "t.priority_color, t.deadline, t.is_completed, "
                  "t.created_at, t.updated_at, t.deleted_at, t.sync_status, t.server_version "
                  "FROM tasks t "
                  "JOIN statuses s ON s.id = t.status_id "
                  "WHERE t.board_id = :board_id AND t.deleted_at IS NULL AND s.deleted_at IS NULL");

    query.bindValue(":board_id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error find by board_id: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalTask> tasks;
    while (query.next()) {
        tasks.push_back(createTask(query));
    }
    return tasks;
}

int LocalTaskRepository::createLocalId() {
    QSqlQuery query(db_);
    if (!query.exec("SELECT MIN(id) FROM tasks")) {
        return -1;
    }

    if (!query.next()) {
        return -1;
    }

    const int min_id = query.value(0).toInt();
    return min_id < 0 ? min_id - 1 : -1;
}

void LocalTaskRepository::replaceId(int old_id, int new_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE tasks SET id = :new_id WHERE id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: replaceId error: " + query.lastError().text()).toStdString());
    }
}

void LocalTaskRepository::deleteById(int task_id) {
    QSqlQuery query(db_);
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error purge by id " + query.lastError().text()).toStdString());
    }
}

void LocalTaskRepository::markDeletedById(int task_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATE tasks "
                  "SET deleted_at = CURRENT_TIMESTAMP, sync_status = 'pending' "
                  "WHERE id = :id");

    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error delete by id " + query.lastError().text()).toStdString());
    }
}

void LocalTaskRepository::markSynced(int task_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE tasks SET sync_status = 'synced' WHERE id = :id");
    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error mark synced " + query.lastError().text()).toStdString());
    }
}

std::vector<LocalTask> LocalTaskRepository::findUnsynced() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "is_completed, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM tasks "
                  "WHERE sync_status = 'pending' ");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error find unsynced tasks " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalTask> tasks;
    while (query.next()) {
        tasks.push_back(createTask(query));
    }

    return tasks;
}
