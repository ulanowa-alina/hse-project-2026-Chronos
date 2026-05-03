#include "local_status_repository.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

LocalStatus createStatus(QSqlQuery& query) {
    return LocalStatus(query.value("id").toInt(), query.value("board_id").toInt(),
                       query.value("name").toString(), query.value("position").toInt(),
                       query.value("is_sync").toInt(), query.value("is_deleted").toInt());
}

LocalSatusRepository::LocalSatusRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalStatus LocalSatusRepository::insert(const LocalStatus& status) {
    QSqlQuery query(db_);
    query.prepare("INSERT INTO statuses (id, board_id, name, position, is_sync, is_deleted)"
                  "VALUES(:id, :board_id, :name, :position, :is_sync, :is_deleted)");
    query.bindValue(":id", status.id_);
    query.bindValue(":board_id", status.board_id_);
    query.bindValue(":name", status.name_);
    query.bindValue(":position", status.position_);
    query.bindValue(":is_sync", status.is_sync_);
    query.bindValue(":is_deleted", status.is_deleted_);

    if (!query.exec()) {
        qDebug() << "LocalStatusRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalTaskRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return status;
}

LocalStatus LocalSatusRepository::update(const LocalStatus& status) {
    QSqlQuery query(db_);
    query.prepare("UPDATE statuses SET (id, board_id, name, position, is_sync, is_deleted)"
                  "VALUES(:id, :board_id, :name, :position, :is_sync, :is_deleted)"
                  "WHERE id = :id");
    query.bindValue(":id", status.id_);
    query.bindValue(":board_id", status.board_id_);
    query.bindValue(":name", status.name_);
    query.bindValue(":position", status.position_);
    query.bindValue(":is_sync", status.is_sync_);
    query.bindValue(":is_deleted", status.is_deleted_);

    if (!query.exec()) {
        qDebug() << "LocalStatusRepository: update error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalStatusRepository: update error: " + query.lastError().text()).toStdString());
    }

    return status;
}

LocalStatus LocalSatusRepository::save(const LocalStatus& status) {
    try {
        if (status.id_ == 0) {
            return insert(status);
        } else {
            return update(status);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Status: ") + e.what());
    }
}

std::optional<LocalStatus> LocalSatusRepository::findByid(int status_id) {
    QSqlQuery query(db_);

    query.prepare(
        "SELECT id, board_id, name, position, is_sync, is_deleted FROM statuses WHERE id = :id");

    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find by id: " + query.lastError().text()).toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createStatus(query);
}

std::vector<LocalStatus> LocalSatusRepository::findByBoardId(int board_id) {

    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, is_sync, is_deleted FROM statuses WHERE id "
                  "= :id AND is_deleted = 0 ");

    query.bindValue(":board_id", board_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalBoardRepository: Error find by board_id: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalStatus> statuses;
    while (query.next()) {
        statuses.push_back(createStatus(query));
    }
    return statuses;
}

void LocalSatusRepository::deleteById(int status_id) {
    QSqlQuery query(db_);

    query.prepare("UPDATE tasks SET is_deleted = 1, is_sync = 0 WHERE id = :id");

    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error delete by id " + query.lastError().text())
                .toStdString());
    }
}

void LocalSatusRepository::markSynced(int status_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE tasks SET is_sync = 1 WHERE id = :id");
    query.bindValue(":id", status_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Erorr mark synced " + query.lastError().text()).toStdString());
    }
}

std::vector<LocalStatus> LocalSatusRepository::findUnsynced() {
    QSqlQuery query(db_);

    query.prepare("SELECT id, board_id, name, position, is_sync, is_deleted "
                  "FROM statuses WHERE is_sync = 0 ");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalStatusRepository: Error find unsynced tasks " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalStatus> statuses;
    while (query.next()) {
        statuses.push_back(createStatus(query));
    }

    return statuses;
}
