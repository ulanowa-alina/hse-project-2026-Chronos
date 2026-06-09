#include "local_board_repository.hpp"

#include "repository_utils.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalBoard createBoard(const QSqlQuery& query) {
    return LocalBoard(query.value("id").toInt(), query.value("user_id").toInt(),
                      query.value("title").toString(), query.value("description").toString(),
                      query.value("created_at").toString(), query.value("updated_at").toString(),
                      query.value("deleted_at").toString(),
                      stringToSyncStatus(query.value("sync_status").toString()),
                      query.value("server_version").toInt());
}

LocalBoardRepository::LocalBoardRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalBoard LocalBoardRepository::insert(const LocalBoard& board) {
    if (board.title_.trimmed().isEmpty()) {
        throw std::runtime_error("LocalBoardRepository: title must not be empty");
    }

    const QString created_at = processingTimestamp(board.created_at_);
    const QString updated_at =
        processingTimestamp(board.updated_at_.isEmpty() ? created_at : board.updated_at_);

    QSqlQuery query(db_);

    query.prepare("INSERT INTO boards ("
                  "id, user_id, title, description, "
                  "created_at, updated_at, deleted_at, sync_status, server_version"
                  ") VALUES ("
                  ":id, :user_id, :title, :description, "
                  ":created_at, :updated_at, :deleted_at, :sync_status, :server_version"
                  ")");

    query.bindValue(":id", board.id_);
    query.bindValue(":user_id", board.user_id_);
    query.bindValue(":title", board.title_.trimmed());
    query.bindValue(":description", board.description_);
    query.bindValue(":created_at", created_at);
    query.bindValue(":updated_at", updated_at);
    query.bindValue(":deleted_at", board.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : board.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(board.sync_status_));
    query.bindValue(":server_version", board.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalBoardRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalBoardRepository: insert error: " + query.lastError().text()).toStdString());
    }

    LocalBoard saved = board;
    saved.title_ = board.title_.trimmed();
    saved.created_at_ = created_at;
    saved.updated_at_ = updated_at;
    return saved;
}

LocalBoard LocalBoardRepository::update(const LocalBoard& board) {
    if (board.title_.trimmed().isEmpty()) {
        throw std::runtime_error("LocalBoardRepository: title must not be empty");
    }

    const QString updated_at = processingTimestamp(board.updated_at_);

    QSqlQuery query(db_);

    query.prepare("UPDATE boards SET "
                  "user_id = :user_id, "
                  "title = :title, "
                  "description = :description, "
                  "created_at = :created_at, "
                  "updated_at = :updated_at, "
                  "deleted_at = :deleted_at, "
                  "sync_status = :sync_status, "
                  "server_version = :server_version "
                  "WHERE id = :id");

    query.bindValue(":id", board.id_);
    query.bindValue(":user_id", board.user_id_);
    query.bindValue(":title", board.title_.trimmed());
    query.bindValue(":description", board.description_);
    query.bindValue(":created_at", processingTimestamp(board.created_at_));
    query.bindValue(":updated_at", updated_at);
    query.bindValue(":deleted_at", board.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : board.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(board.sync_status_));
    query.bindValue(":server_version", board.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalBoardRepository: update error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalBoardRepository: update error: " + query.lastError().text()).toStdString());
    }

    return board;
}

LocalBoard LocalBoardRepository::save(const LocalBoard& board) {
    try {
        if (!findById(board.id_)) {
            return insert(board);
        }
        return update(board);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Board: ") + e.what());
    }
}

std::optional<LocalBoard> LocalBoardRepository::findById(int board_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, user_id, title, description, "
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
    query.prepare("SELECT id, user_id, title, description, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM boards WHERE deleted_at IS NULL");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find all boards: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalBoard> boards;
    while (query.next()) {
        boards.push_back(createBoard(query));
    }

    return boards;
}

std::optional<int> LocalBoardRepository::findFirstBoard() {
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM boards WHERE deleted_at IS NULL ORDER BY id LIMIT 1");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find first board: " + query.lastError().text())
                .toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return query.value(0).toInt();
}

int LocalBoardRepository::createLocalId() {
    QSqlQuery query(db_);
    if (!query.exec("SELECT MIN(id) FROM boards")) {
        return -1;
    }

    if (!query.next()) {
        return -1;
    }

    const int min_id = query.value(0).toInt();
    return min_id < 0 ? min_id - 1 : -1;
}

void LocalBoardRepository::replaceId(int old_id, int new_id) {
    QSqlQuery query(db_);
    if (!db_.transaction()) {
        throw std::runtime_error("LocalBoardRepository: failed to start transaction");
    }

    query.prepare("UPDATE statuses SET board_id = :new_id WHERE board_id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);
    if (!query.exec()) {
        db_.rollback();
        throw std::runtime_error(
            ("LocalBoardRepository: replaceId statuses error: " + query.lastError().text())
                .toStdString());
    }

    query.prepare("UPDATE tasks SET board_id = :new_id WHERE board_id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);
    if (!query.exec()) {
        db_.rollback();
        throw std::runtime_error(
            ("LocalBoardRepository: replaceId tasks error: " + query.lastError().text())
                .toStdString());
    }

    query.prepare("UPDATE boards SET id = :new_id WHERE id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);
    if (!query.exec()) {
        db_.rollback();
        throw std::runtime_error(
            ("LocalBoardRepository: replaceId boards error: " + query.lastError().text())
                .toStdString());
    }

    if (!db_.commit()) {
        throw std::runtime_error("LocalBoardRepository: failed to commit replaceId");
    }
}

void LocalBoardRepository::deleteById(int board_id) {
    QSqlQuery query(db_);
    query.prepare("DELETE FROM boards WHERE id = :id");
    query.bindValue(":id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error purge by id: " + query.lastError().text()).toStdString());
    }
}

void LocalBoardRepository::markDeletedById(int board_id) {
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

    query.prepare("SELECT id, user_id, title, description,"
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
