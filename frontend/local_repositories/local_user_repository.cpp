#include "local_user_repository.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

LocalUser createUser(const QSqlQuery& query) {
    return LocalUser(query.value("id").toInt(), query.value("email").toString(),
                     query.value("name").toString(), query.value("status").toString(),
                     query.value("created_at").toString(), query.value("updated_at").toString(),
                     query.value("deleted_at").toString(),
                     stringToSyncStatus(query.value("sync_status").toString()),
                     query.value("server_version").toInt());
}

LocalUserRepository::LocalUserRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalUser LocalUserRepository::insert(const LocalUser& user) {
    QSqlQuery query(db_);

    query.prepare("INSERT INTO users ("
                  "id, email, name, status, "
                  "created_at, updated_at, deleted_at, sync_status, server_version"
                  ") VALUES ("
                  ":id, :email, :name, :status, "
                  ":created_at, :updated_at, :deleted_at, :sync_status, :server_version"
                  ")");

    query.bindValue(":id", user.id_);
    query.bindValue(":email", user.email_);
    query.bindValue(":name", user.name_);
    query.bindValue(":status", user.status_);
    query.bindValue(":created_at", user.created_at_);
    query.bindValue(":updated_at", user.updated_at_);
    query.bindValue(":deleted_at", user.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : user.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(user.sync_status_));
    query.bindValue(":server_version", user.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalUserRepository: insert error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalUserRepository: insert error: " + query.lastError().text()).toStdString());
    }

    return user;
}

LocalUser LocalUserRepository::update(const LocalUser& user) {
    QSqlQuery query(db_);

    query.prepare("UPDATE users SET "
                  "email = :email, "
                  "name = :name, "
                  "status = :status, "
                  "created_at = :created_at, "
                  "updated_at = :updated_at, "
                  "deleted_at = :deleted_at, "
                  "sync_status = :sync_status, "
                  "server_version = :server_version "
                  "WHERE id = :id");

    query.bindValue(":id", user.id_);
    query.bindValue(":email", user.email_);
    query.bindValue(":name", user.name_);
    query.bindValue(":status", user.status_);
    query.bindValue(":created_at", user.created_at_);
    query.bindValue(":updated_at", user.updated_at_);
    query.bindValue(":deleted_at", user.deleted_at_.isEmpty()
                                       ? QVariant(QMetaType(QMetaType::QString))
                                       : user.deleted_at_);
    query.bindValue(":sync_status", syncStatusToString(user.sync_status_));
    query.bindValue(":server_version", user.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalUserRepository: update error:" << query.lastError().text();
        throw std::runtime_error(
            ("LocalUserRepository: update error: " + query.lastError().text()).toStdString());
    }

    return user;
}

LocalUser LocalUserRepository::save(const LocalUser& user) {
    try {
        if (!findById(user.id_)) {
            return insert(user);
        }
        return update(user);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save User: ") + e.what());
    }
}

std::optional<LocalUser> LocalUserRepository::findById(int user_id) {
    QSqlQuery query(db_);
    query.prepare("SELECT id, email, name, status, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM users WHERE id = :id");
    query.bindValue(":id", user_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalUserRepository: Error find by id: " + query.lastError().text()).toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createUser(query);
}

std::optional<LocalUser> LocalUserRepository::getCurrentUser() {
    QSqlQuery query(db_);
    query.prepare("SELECT id, email, name, status, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM users WHERE deleted_at IS NULL ORDER BY id LIMIT 1");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalUserRepository: Error get current user: " + query.lastError().text())
                .toStdString());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return createUser(query);
}

std::vector<LocalUser> LocalUserRepository::findUnsynced() {
    QSqlQuery query(db_);
    query.prepare("SELECT id, email, name, status, "
                  "created_at, updated_at, deleted_at, sync_status, server_version "
                  "FROM users WHERE sync_status = 'pending'");

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalUserRepository: Error find unsynced: " + query.lastError().text())
                .toStdString());
    }

    std::vector<LocalUser> users;
    while (query.next()) {
        users.push_back(createUser(query));
    }

    return users;
}

void LocalUserRepository::markSynced(int user_id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE users SET sync_status = 'synced' WHERE id = :id");
    query.bindValue(":id", user_id);

    if (!query.exec()) {
        throw std::runtime_error(
            ("LocalUserRepository: Error mark synced: " + query.lastError().text()).toStdString());
    }
}
