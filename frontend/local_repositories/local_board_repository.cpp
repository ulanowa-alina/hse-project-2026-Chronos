#include "local_board_repository.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalBoard createBoard(const QSqlQuery& query) {
    return LocalBoard(query.value("id").toInt(), query.value("title").toString(),
                      query.value("description").toString(), query.value("is_private").toInt(),
                      query.value("created_at").toString(), query.value("updated_at").toString(),
                      query.value("is_sync").toInt(), query.value("is_deleted").toInt());
}

LocalBoardRepository::LocalBoardRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalBoard LocalBoardRepository::insert(const LocalBoard& board) {
    QSqlQuery query(db_);

    query.prepare("INSERT INTO tasks ("
                  "id, title, description, is_private, "
                  "created_at, updated_at, is_sync, is_deleted"
                  ") VALUES ("
                  ":id, :title, :description, :is_private, "
                  ":created_at, :updated_at, :is_sync, :is_deleted"
                  ")");

    query.bindValue(":id", board.id_);
    query.bindValue(":title", board.title_);
    query.bindValue(":description", board.description_);
    query.bindValue(":is_private", board.is_private_);
    query.bindValue(":created_at", board.created_at_);
    query.bindValue(":updated_at", board.updated_at_);
    query.bindValue(":is_sync", board.is_sync_);
    query.bindValue(":is_deleted", board.is_deleted_);

    if (!query.exec()) {
        qDebug() << "LocalBoardRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalBoardRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return board;
}

LocalBoard LocalBoardRepository::update(const LocalBoard& board) {
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
                  "is_deleted = :is_deleted "
                  "WHERE id = :id");

    query.bindValue(":id", board.id_);
    query.bindValue(":title", board.title_);
    query.bindValue(":description", board.description_);
    query.bindValue(":is_private", board.is_private_);
    query.bindValue(":created_at", board.created_at_);
    query.bindValue(":updated_at", board.updated_at_);
    query.bindValue(":is_sync", board.is_sync_);
    query.bindValue(":is_deleted", board.is_deleted_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return board;
}

LocalBoard LocalBoardRepository::save(const LocalBoard& board) {
    try {
        if (board.id_ == 0) {
            return insert(board);
        } else {
            return update(board);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Board: ") + e.what());
    }
}

std::optional<LocalBoard> LocalBoardRepository::findById(int board_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, title, description, is_private, "
                  "created_at, updated_at, is_sync, is_deleted "
                  "FROM tasks WHERE id = :id");
    query.bindValue(":id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find by id: " + query.lastError().text()).toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createBoard(query);
}

std::vector<LocalBoard> LocalBoardRepository::findAll(int user_id) {
    QSqlQuery query(db_);

    query.prepare("SELECT id, title, description, is_private,"
                  "created_at, updated_at, is_sync, is_deleted"
                  "FROM boards");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find all: " + query.lastError().text()).toStdString());
    }

    std::vector<LocalBoard> boards;
    while (query.next()) {
        boards.push_back(createBoard(query));
    }

    return boards;
}

void LocalBoardRepository::deleteById(int board_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATED boards SET is_deleted = 1, is_sync = 0 WHERE id = :id");
    query.bindValue(":id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error delete by id: " + query.lastError().text())
                .toStdString());
    }
}

std::vector<LocalBoard> LocalBoardRepository::findUnsynced() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, title, description, is_private,"
                  "created_at, updated_at, is_sync, is_deleted"
                  "FROM boards WHERE is_sinc = 0");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find unsynced boards: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalBoard> boards;

    while (query.next()) {
        boards.push_back(createBoard(query));
    }

    return boards;
}

void LocalBoardRepository::markSynced(int board_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATE boards SET is_sync = 1 WHERE id = :id");
    query.bindValue(":id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error mark synced: " + query.lastError().text()).toStdString());
    }
}
