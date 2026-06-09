#include "user_sync_manager.hpp"

#include "sync_json_helpers.hpp"

#include <QDebug>
#include <QJsonDocument>

UserSyncManager::UserSyncManager(QSqlDatabase& db, NetworkManager* network_manager)
    : db_(db)
    , network_manager_(network_manager) {
}

QString UserSyncManager::modelName() const {
    return "user";
}

QString UserSyncManager::loadEndpoint() const {
    return network_manager_->user_info_url_;
}

bool UserSyncManager::handlesLoadEndpoint(const QString& endpoint) const {
    return endpoint == network_manager_->user_info_url_;
}

LocalUser UserSyncManager::userFromJson(const QJsonObject& obj) const {
    const QString created_at = jsonTimestamp(obj, "created_at");
    const QString updated_at = jsonTimestamp(obj, "updated_at", "created_at");
    const QString password_hash = obj.value("password_hash").toString();
    return LocalUser(obj["id"].toInt(), obj["email"].toString(), obj["name"].toString(),
                     obj["status"].toString(), password_hash, created_at, updated_at, QString(),
                     SyncStatus::SYNCED, 1);
}

void UserSyncManager::saveFromServer(const LocalUser& user) {
    LocalUserRepository repo(db_);
    const auto existing = repo.findById(user.id_);
    if (existing && existing->sync_status_ == SyncStatus::PENDING &&
        existing->updated_at_ > user.updated_at_) {
        LocalUser conflict = *existing;
        conflict.sync_status_ = SyncStatus::CONFLICT;
        repo.save(conflict);
        return;
    }

    try {
        repo.save(user);
    } catch (const std::exception& e) {
        qDebug() << "UserSyncManager: failed to save user:" << e.what();
    }
}

void UserSyncManager::saveFromLogin(const QJsonObject& user_obj) {
    saveFromServer(userFromJson(user_obj));
}

void UserSyncManager::setPassword(const QString& password) {
    pending_password_ = password;
}

void UserSyncManager::parse(const QJsonArray& data) {
    for (const auto& value : data) {
        if (!value.isObject()) {
            continue;
        }
        saveFromServer(userFromJson(value.toObject()));
    }
}

void UserSyncManager::parseObject(const QJsonObject& data) {
    saveFromServer(userFromJson(data));
}

void UserSyncManager::load() {
    network_manager_->GET(network_manager_->user_info_url_);
}

void UserSyncManager::sync() {
    LocalUserRepository repo(db_);
    const std::vector<LocalUser> pending = repo.findUnsynced();

    for (const auto& user : pending) {
        if (user.email_.trimmed().isEmpty() || user.name_.trimmed().isEmpty() ||
            user.status_.trimmed().isEmpty()) {
            continue;
        }

        QJsonObject json;
        json["name"] = user.name_;
        json["email"] = user.email_;
        json["status"] = user.status_;
        if (!pending_password_.isEmpty()) {
            json["password"] = pending_password_;
            pending_password_.clear();
        }
        network_manager_->syncPUT(network_manager_->user_edit_info_url_, json, modelName(),
                                  user.id_, "update");
    }
}

bool UserSyncManager::handlePushResponse(const QString& endpoint, const QByteArray& data, int code,
                                         int local_id, const QString& operation) {
    if (code < 200 || code >= 300) {
        return false;
    }

    if (operation == "update") {
        const QJsonObject obj = QJsonDocument::fromJson(data).object()["data"].toObject();
        saveFromServer(userFromJson(obj));
        LocalUserRepository repo(db_);
        repo.markSynced(local_id);
        return true;
    }

    return false;
}
