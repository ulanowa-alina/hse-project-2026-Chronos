#include "local_pomodoro_session_repository.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

LocalPomodoroSessionRepository::LocalPomodoroSessionRepository(QSqlDatabase& db)
    : db_(db) {
}

LocalPomodoroSession LocalPomodoroSessionRepository::fromQuery(const QSqlQuery& query) {
    return LocalPomodoroSession(
        query.value("id").toInt(), query.value("user_id").toInt(),
        query.value("work_duration_seconds").toInt(), query.value("break_duration_seconds").toInt(),
        query.value("goal_minutes").toInt(), query.value("completed_cycles").toInt(),
        query.value("started_at").toString(), query.value("completed_at").toString(),
        query.value("created_at").toString(), query.value("updated_at").toString(),
        query.value("deleted_at").toString(),
        stringToSyncStatus(query.value("sync_status").toString()),
        query.value("server_version").toInt());
}

std::optional<LocalPomodoroSession> LocalPomodoroSessionRepository::findById(int id) {
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM pomodoro_sessions WHERE id = ? AND deleted_at IS NULL");
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return fromQuery(query);
}

std::vector<LocalPomodoroSession> LocalPomodoroSessionRepository::findByUserId(int user_id) {
    std::vector<LocalPomodoroSession> sessions;
    QSqlQuery query(db_);
    query.prepare("SELECT * FROM pomodoro_sessions WHERE user_id = ? AND deleted_at IS NULL ORDER "
                  "BY created_at DESC");
    query.addBindValue(user_id);

    if (!query.exec()) {
        qDebug() << "LocalPomodoroSessionRepository: Error finding sessions by user_id:"
                 << query.lastError().text();
        return sessions;
    }

    while (query.next()) {
        sessions.push_back(fromQuery(query));
    }

    return sessions;
}

std::vector<LocalPomodoroSession> LocalPomodoroSessionRepository::findAll() {
    std::vector<LocalPomodoroSession> sessions;
    QSqlQuery query(db_);
    query.prepare(
        "SELECT * FROM pomodoro_sessions WHERE deleted_at IS NULL ORDER BY created_at DESC");

    if (!query.exec()) {
        qDebug() << "LocalPomodoroSessionRepository: Error finding all sessions:"
                 << query.lastError().text();
        return sessions;
    }

    while (query.next()) {
        sessions.push_back(fromQuery(query));
    }

    return sessions;
}

bool LocalPomodoroSessionRepository::insert(const LocalPomodoroSession& session) {
    QSqlQuery query(db_);
    query.prepare(
        "INSERT INTO pomodoro_sessions (id, user_id, goal_minutes, work_duration_seconds, "
        "break_duration_seconds, completed_cycles, started_at, completed_at, created_at, "
        "updated_at, "
        "sync_status, server_version) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(session.id_);
    query.addBindValue(session.user_id_);
    query.addBindValue(session.goal_minutes_);
    query.addBindValue(session.work_duration_seconds_);
    query.addBindValue(session.break_duration_seconds_);
    query.addBindValue(session.completed_cycles_);
    query.addBindValue(session.started_at_);
    query.addBindValue(session.completed_at_);
    query.addBindValue(session.created_at_);
    query.addBindValue(session.updated_at_);
    query.addBindValue(syncStatusToString(session.sync_status_));
    query.addBindValue(session.server_version_);

    if (!query.exec()) {
        qDebug() << "LocalPomodoroSessionRepository: Error inserting session:"
                 << query.lastError().text();
        return false;
    }

    return true;
}

bool LocalPomodoroSessionRepository::update(const LocalPomodoroSession& session) {
    QSqlQuery query(db_);
    query.prepare(
        "UPDATE pomodoro_sessions SET user_id = ?, goal_minutes = ?, work_duration_seconds = ?, "
        "break_duration_seconds = ?, completed_cycles = ?, started_at = ?, completed_at = ?, "
        "updated_at = ?, sync_status = ?, server_version = ? WHERE id = ?");
    query.addBindValue(session.user_id_);
    query.addBindValue(session.goal_minutes_);
    query.addBindValue(session.work_duration_seconds_);
    query.addBindValue(session.break_duration_seconds_);
    query.addBindValue(session.completed_cycles_);
    query.addBindValue(session.started_at_);
    query.addBindValue(session.completed_at_);
    query.addBindValue(session.updated_at_);
    query.addBindValue(syncStatusToString(session.sync_status_));
    query.addBindValue(session.server_version_);
    query.addBindValue(session.id_);

    if (!query.exec()) {
        qDebug() << "LocalPomodoroSessionRepository: Error updating session:"
                 << query.lastError().text();
        return false;
    }

    return true;
}

bool LocalPomodoroSessionRepository::remove(int id) {
    QSqlQuery query(db_);
    query.prepare("UPDATE pomodoro_sessions SET deleted_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "LocalPomodoroSessionRepository: Error removing session:"
                 << query.lastError().text();
        return false;
    }

    return true;
}

int LocalPomodoroSessionRepository::getNextId() {
    QSqlQuery query(db_);
    query.prepare("SELECT MAX(id) FROM pomodoro_sessions");

    if (!query.exec() || !query.next()) {
        return 1;
    }

    int max_id = query.value(0).toInt();
    return max_id + 1;
}
