#ifndef LOCAL_TASK_HPP
#define LOCAL_TASK_HPP

#include "sync_status.hpp"

#include <QString>

struct LocalTask {
    int id_;
    int board_id_;
    QString title_;
    QString description_;
    int status_id_;
    QString priority_color_;
    QString deadline_;
    bool is_completed_;
    QString created_at_;
    QString updated_at_;
    QString deleted_at_;
    SyncStatus sync_status_;
    int server_version_;

    LocalTask() = default;

    LocalTask(int id, int board_id, const QString& title, int status_id,
              const QString& priority_color, const QString& description = QString(),
              const QString& deadline = QString(), bool is_completed = false,
              const QString& created_at = QString(), const QString& updated_at = QString(),
              const QString& deleted_at = QString(),
              const SyncStatus& sync_status = SyncStatus::PENDING, int server_version = 0)
        : id_(id)
        , board_id_(board_id)
        , title_(title)
        , description_(description)
        , status_id_(status_id)
        , priority_color_(priority_color)
        , deadline_(deadline)
        , is_completed_(is_completed)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , deleted_at_(deleted_at)
        , sync_status_(sync_status)
        , server_version_(server_version) {
    }
};

#endif // LOCAL_TASK_HPP
