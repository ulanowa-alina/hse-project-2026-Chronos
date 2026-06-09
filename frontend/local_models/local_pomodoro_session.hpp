#ifndef LOCAL_POMODORO_SESSION_HPP
#define LOCAL_POMODORO_SESSION_HPP

#include "sync_status.hpp"

#include <QString>

struct LocalPomodoroSession {
    int id_;
    int user_id_;
    int goal_minutes_;
    int work_duration_seconds_;
    int break_duration_seconds_;
    int completed_cycles_;
    QString started_at_;
    QString completed_at_;
    QString created_at_;
    QString updated_at_;
    QString deleted_at_;
    SyncStatus sync_status_;
    int server_version_;

    LocalPomodoroSession() = default;

    LocalPomodoroSession(
        int id, int user_id, int work_duration_seconds, int break_duration_seconds,
        int goal_minutes = 0, int completed_cycles = 0, const QString& started_at = QString(),
        const QString& completed_at = QString(), const QString& created_at = QString(),
        const QString& updated_at = QString(), const QString& deleted_at = QString(),
        const SyncStatus& sync_status = SyncStatus::PENDING, int server_version = 0)
        : id_(id)
        , user_id_(user_id)
        , goal_minutes_(goal_minutes)
        , work_duration_seconds_(work_duration_seconds)
        , break_duration_seconds_(break_duration_seconds)
        , completed_cycles_(completed_cycles)
        , started_at_(started_at)
        , completed_at_(completed_at)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , deleted_at_(deleted_at)
        , sync_status_(sync_status)
        , server_version_(server_version) {
    }
};

#endif // LOCAL_POMODORO_SESSION_HPP
