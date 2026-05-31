#include "local_status_repository.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

LocalStatus createStatus(const QSqlQuery& query) {
    return LocalStatus(query.value("id").toInt(), query.value("board_id").toInt(),
                       query.value("name").toString(), query.value("position").toInt(),
                       query.value("created_at").toString(), query.value("updated_at").toString(),
                       query.value("deleted_at").toString(),
                       stringToSyncStatus(query.value("sync_status").toString()),
                       query.value("server_version").toInt());
}

LocalStatusRepository::LocalStatusRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalStatus LocalStatusRepository::insert(const LocalStatus& status) {
    QSqlQuery query(db_);
    query.prepare("INSERT INTO statuses ("
                  "id, board_id, name, position, created_at, updated_at, deleted_at, "
                  "sync_status, server_version"
                  ") VALUES("
                  ":id, :board_id, :name, :position, :created_at, :updated_at, :deleted_at, "
                  ":sync_status, :server_version)");
    query.bindValue(":id", status.id_);
    query.bindValue(":board_id", status.board_id_);
    query.bindValue(":name", status.name_);
    query.bindValue(":position", status.position_);
    query.bindValue(":created_at", status.created_at_);
    query.bindValue(":updated_at", status.updated_at_);
    query.bindValue(":deleted_at", status.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : status.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(status.sync_status_));
    query.bindValue(":server_version", status.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalStatusRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalStatusRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return status;
}

LocalStatus LocalStatusRepository::update(const LocalStatus& status) {
    QSqlQuery query(db_);
    query.prepare("UPDATE statuses SET "
                  "board_id = :board_id, "
                  "name = :name, "
                  "position = :position, "
                  "created_at = :created_at, "
                  "updated_at = :updated_at, "
                  "deleted_at = :deleted_at, "
                  "sync_status = :sync_status, "
                  "server_version = :server_version "
                  "WHERE id = :id");
    query.bindValue(":id", status.id_);
    query.bindValue(":board_id", status.board_id_);
    query.bindValue(":name", status.name_);
    query.bindValue(":position", status.position_);
    query.bindValue(":created_at", status.created_at_);
    query.bindValue(":updated_at", status.updated_at_);
    query.bindValue(":deleted_at", status.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : status.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(status.sync_status_));
    query.bindValue(":server_version", status.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalStatusRepository: update error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalStatusRepository: update error: " + query.lastError().text()).toStdString());
    }

    return status;
}

LocalStatus LocalStatusRepository::save(const LocalStatus& status) {
    try {
        if (!findById(status.id_)) {
            return insert(status);
        }
        return update(status);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Status: ") + e.what());
    }
}

std::optional<LocalStatus> LocalStatusRepository::findById(int status_id) {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, created_at, updated_at, deleted_at, "
                  "sync_status, server_version FROM statuses "
                  "WHERE id = :id");

    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error find by id: " + query.lastError().text()).toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createStatus(query);
}

std::vector<LocalStatus> LocalStatusRepository::findAll() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, created_at, updated_at, deleted_at, "
                  "sync_status, server_version FROM statuses "
                  "WHERE deleted_at IS NULL");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error find all: " + query.lastError().text()).toStdString());
    }

    std::vector<LocalStatus> statuses;
    while (query.next()) {
        statuses.push_back(createStatus(query));
    }
    return statuses;
}

std::vector<LocalStatus> LocalStatusRepository::findByBoardId(int board_id) {

    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, created_at, updated_at, deleted_at, "
                  "sync_status, server_version FROM statuses "
                  "WHERE board_id = :board_id AND deleted_at IS NULL "
                  "ORDER BY position");

    query.bindValue(":board_id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error find by board_id: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalStatus> statuses;
    while (query.next()) {
        statuses.push_back(createStatus(query));
    }
    return statuses;
}

int LocalStatusRepository::createLocalId() {
    QSqlQuery query(db_);
    if (!query.exec("SELECT MIN(id) FROM statuses")) {
        return -1;
    }

    if (!query.next()) {
        return -1;
    }

    const int min_id = query.value(0).toInt();
    return min_id < 0 ? min_id - 1 : -1;
}

void LocalStatusRepository::replaceId(int old_id, int new_id) {
    QSqlQuery query(db_);
    if (!db_.transaction()) {
        throw std::runtime_error("LocalStatusRepository: failed to start transaction");
    }

    query.prepare("UPDATE tasks SET status_id = :new_id WHERE status_id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);
    if (!query.exec()) {
        db_.rollback();
        throw std::runtime_error(
            ("LocalStatusRepository: replaceId tasks error: " + query.lastError().text())
                .toStdString());
    }

    query.prepare("UPDATE statuses SET id = :new_id WHERE id = :old_id");
    query.bindValue(":new_id", new_id);
    query.bindValue(":old_id", old_id);
    if (!query.exec()) {
        db_.rollback();
        throw std::runtime_error(
            ("LocalStatusRepository: replaceId statuses error: " + query.lastError().text())
                .toStdString());
    }

    if (!db_.commit()) {
        throw std::runtime_error("LocalStatusRepository: failed to commit replaceId");
    }
}

void LocalStatusRepository::deleteById(int status_id) {
    QSqlQuery query(db_);
    query.prepare("DELETE FROM statuses WHERE id = :id");
    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error purge by id " + query.lastError().text()).toStdString());
    }
}

void LocalStatusRepository::markDeletedById(int status_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATE statuses "
                  "SET deleted_at = CURRENT_TIMESTAMP, sync_status = 'pending' "
                  "WHERE id = :id");

    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error delete by id " + query.lastError().text())
                .toStdString());
    }
}

void LocalStatusRepository::markSynced(int status_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE statuses SET sync_status = 'synced' WHERE id = :id");
    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error mark synced " + query.lastError().text()).toStdString());
    }
}

std::vector<LocalStatus> LocalStatusRepository::findUnsynced() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, created_at, updated_at, deleted_at, "
                  "sync_status, server_version "
                  "FROM statuses WHERE sync_status = 'pending' ");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error find unsynced " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalStatus> statuses;
    while (query.next()) {
        statuses.push_back(createStatus(query));
    }

    return statuses;
}
