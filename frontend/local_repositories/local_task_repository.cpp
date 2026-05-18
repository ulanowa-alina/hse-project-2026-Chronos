#include "local_task_repository.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalTask createTask(const QSqlQuery& query) {
    return LocalTask(query.value("id").toInt(), query.value("board_id").toInt(),
                     query.value("title").toString(), query.value("description").toString(),
                     query.value("status_id").toInt(), query.value("priority_color").toString(),
                     query.value("deadline").toString(), query.value("created_at").toString(),
                     query.value("updated_at").toString(), query.value("is_sync").toInt(),
                     query.value("is_deleted").toInt(), query.value("is_new").toInt());
}

LocalTaskRepository::LocalTaskRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalTask LocalTaskRepository::insert(const LocalTask& task) {
    QSqlQuery query(db_);

    query.prepare("INSERT INTO tasks ("
                  "id, board_id, title, description, status_id, priority_color, deadline, "
                  "created_at, updated_at, is_sync, is_deleted, is_new"
                  ") VALUES ("
                  ":id, :board_id, :title, :description, :status_id, :priority_color, :deadline, "
                  ":created_at, :updated_at, :is_sync, :is_deleted, :is_new"
                  ")");

    query.bindValue(":id", task.id_);
    query.bindValue(":board_id", task.board_id_);
    query.bindValue(":title", task.title_);
    query.bindValue(":description", task.description_);
    query.bindValue(":status_id", task.status_id_);
    query.bindValue(":priority_color", task.priority_color_);
    query.bindValue(":deadline", task.deadline_);
    query.bindValue(":created_at", task.created_at_);
    query.bindValue(":updated_at", task.updated_at_);
    query.bindValue(":is_sync", task.is_sync_);
    query.bindValue(":is_deleted", task.is_deleted_);
    query.bindValue(":is_new", task.is_new_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return task;
}

LocalTask LocalTaskRepository::update(const LocalTask& task) {
    QSqlQuery query(db_);

    query.prepare("UPDATE tasks SET "
                  "board_id = :board_id, "
                  "title = :title, "
                  "description = :description, "
                  "status_id = :status_id, "
                  "priority_color = :priority_color, "
                  "deadline = :deadline, "
                  "updated_at = :updated_at, "
                  "is_sync = :is_sync, "
                  "is_deleted = :is_deleted, "
                  "is_new = :is_new "
                  "WHERE id = :id");

    query.bindValue(":id", task.id_);
    query.bindValue(":board_id", task.board_id_);
    query.bindValue(":title", task.title_);
    query.bindValue(":description", task.description_);
    query.bindValue(":status_id", task.status_id_);
    query.bindValue(":priority_color", task.priority_color_);
    query.bindValue(":deadline", task.deadline_);
    query.bindValue(":created_at", task.created_at_);
    query.bindValue(":updated_at", task.updated_at_);
    query.bindValue(":is_sync", task.is_sync_);
    query.bindValue(":is_deleted", task.is_deleted_);
    query.bindValue(":is_new", task.is_new_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return task;
}

LocalTask LocalTaskRepository::save(const LocalTask& task) {
    try {
        if (!findById(task.id_)) {
            return insert(task);
        } else {
            return update(task);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Task: ") + e.what());
    }
}

std::optional<LocalTask> LocalTaskRepository::findById(int task_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "created_at, updated_at, is_sync, is_deleted, is_new "
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

std::vector<LocalTask> LocalTaskRepository::findByBoardId(int board_id) {

    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "created_at, updated_at, is_sync, is_deleted, is_new "
                  "FROM tasks "
                  "WHERE board_id = :board_id AND is_deleted = 0 ");

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

std::vector<LocalTask> LocalTaskRepository::findByStatusId(int status_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "created_at, updated_at, is_sync, is_deleted, is_new "
                  "FROM tasks "
                  "WHERE status_id = :status_id AND is_deleted = 0 ");
    query.bindValue(":status_id", status_id);

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

void LocalTaskRepository::deleteById(int task_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATE tasks SET is_deleted = 1, is_sync = 0 WHERE id = :id");

    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Error delete by id " + query.lastError().text()).toStdString());
    }
}

void LocalTaskRepository::markSynced(int task_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE tasks SET is_sync = 1 WHERE id = :id");
    query.bindValue(":id", task_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalTaskRepository: Erorr mark synced " + query.lastError().text()).toStdString());
    }
}

std::vector<LocalTask> LocalTaskRepository::findUnsynced() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, title, description, status_id, priority_color, deadline, "
                  "created_at, updated_at, is_sync, is_deleted, is_new "
                  "FROM tasks "
                  "WHERE is_sync = 0 ");

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
