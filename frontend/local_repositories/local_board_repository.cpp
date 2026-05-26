#include "local_board_repository.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalBoard createBoard(const QSqlQuery& query) {
    return LocalBoard(query.value("id").toInt(), query.value("title").toString(),
                      query.value("description").toString(), query.value("is_private").toInt(),
                      query.value("created_at").toString(), query.value("updated_at").toString(),
                      query.value("deleted_at").toString(), stringToSyncStatus(query.value("sync_status").toString()),
                        query.value("server_version").toInt());
}

LocalBoardRepository::LocalBoardRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalBoard LocalBoardRepository::insert(const LocalBoard& board) {
    QSqlQuery query(db_);

    query.prepare("INSERT INTO boards ("
                  "id, title, description, is_private, "
                  "created_at, updated_at, deleted_at, sync_status, server_version"
                  ") VALUES ("
                  ":id, :title, :description, :is_private, "
                  ":created_at, :updated_at, :deleted_at, :sync_status, :server_version"
                  ")");

    query.bindValue(":id", board.id_);
    query.bindValue(":title", board.title_);
    query.bindValue(":description", board.description_);
    query.bindValue(":is_private", board.is_private_);
    query.bindValue(":created_at", board.created_at_);
    query.bindValue(":updated_at", board.updated_at_);
    query.bindValue(":deleted_at", board.deleted_at_.isEmpty() ? QVariant(QVariant::String)
                                                               : board.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(board.sync_status_));
    query.bindValue(":server_version", board.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalBoardRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalBoardRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return board;
}

LocalBoard LocalBoardRepository::update(const LocalBoard& board) {
    QSqlQuery query(db_);

    query.prepare("UPDATE boards SET "
                  "title = :title, "
                  "description = :description, "
                  "is_private = :is_private, "
                  "created_at = :created_at, "
                  "updated_at = :updated_at, "
                  "deleted_at = :deleted_at, "
                  "sync_status = :sync_status, "
                  "server_version = :server_version "
                  "WHERE id = :id");

    query.bindValue(":id", board.id_);
    query.bindValue(":title", board.title_);
    query.bindValue(":description", board.description_);
    query.bindValue(":is_private", board.is_private_);
    query.bindValue(":created_at", board.created_at_);
    query.bindValue(":updated_at", board.updated_at_);
    query.bindValue(":deleted_at", board.deleted_at_.isEmpty() ? QVariant(QVariant::String)
                                                               : board.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(board.sync_status_));
    query.bindValue(":server_version", board.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalTaskRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return board;
}

LocalBoard LocalBoardRepository::save(const LocalBoard& board) {
    try {
        if (!findById(board.id_)) {
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
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM boards WHERE id = :id");
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

std::vector<LocalBoard> LocalBoardRepository::findAll() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, title, description, is_private,"
                  "created_at, updated_at, deleted_at, sync_status, server_version "
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

    query.prepare("UPDATE boards "
                  "SET deleted_at = CURRENT_TIMESTAMP, sync_status = 'pending' "
                  "WHERE id = :id");
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
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM boards WHERE sync_status = 'pending'");

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

    query.prepare("UPDATE boards SET sync_status = 'synced' WHERE id = :id");
    query.bindValue(":id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error mark synced: " + query.lastError().text()).toStdString());
    }
}
