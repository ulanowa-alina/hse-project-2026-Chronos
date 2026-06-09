#include "pomodoro_sync_manager.hpp"

#include "sync_json_helpers.hpp"

PomodoroSyncManager::PomodoroSyncManager(QSqlDatabase& db, NetworkManager* network_manager)
    : db_(db)
    , network_manager_(network_manager) {
}

QString PomodoroSyncManager::modelName() const {
    return "pomodoro";
}

QString PomodoroSyncManager::loadEndpoint() const {
    return network_manager_->pomodoro_get_user_sessions_url_;
}

bool PomodoroSyncManager::handlesLoadEndpoint(const QString& endpoint) const {
    return endpoint == network_manager_->pomodoro_get_user_sessions_url_;
}

LocalPomodoroSession PomodoroSyncManager::sessionFromJson(const QJsonObject& obj) const {
    const QString started_at = jsonTimestamp(obj, "started_at");
    const QString completed_at =
        obj.value("completed_at").isNull() ? QString() : obj.value("completed_at").toString();
    const int goal_minutes =
        obj.value("goal_minutes").isNull() ? 0 : obj.value("goal_minutes").toInt();

    return LocalPomodoroSession(
        obj["id"].toInt(), obj["user_id"].toInt(), obj["work_duration_seconds"].toInt(),
        obj["break_duration_seconds"].toInt(), goal_minutes, obj["completed_cycles"].toInt(),
        started_at, completed_at, started_at, completed_at.isEmpty() ? started_at : completed_at,
        QString(), SyncStatus::SYNCED, 1);
}

void PomodoroSyncManager::saveFromServer(const LocalPomodoroSession& session) {
    LocalPomodoroSessionRepository repo(db_);
    if (repo.findById(session.id_)) {
        repo.update(session);
    } else {
        repo.insert(session);
    }
}

void PomodoroSyncManager::parse(const QJsonArray& data) {
    for (const auto& value : data) {
        if (!value.isObject()) {
            continue;
        }
        saveFromServer(sessionFromJson(value.toObject()));
    }
}

void PomodoroSyncManager::parseObject(const QJsonObject& data) {
    saveFromServer(sessionFromJson(data));
}

void PomodoroSyncManager::load() {
    network_manager_->GET(network_manager_->pomodoro_get_user_sessions_url_);
}

void PomodoroSyncManager::sync() {
}

bool PomodoroSyncManager::handlePushResponse(const QString&, const QByteArray&, int, int,
                                             const QString&) {
    return false;
}
