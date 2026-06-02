#ifndef LOCAL_STATUS_HPP
#define LOCAL_STATUS_HPP

#include "sync_status.hpp"

#include <QString>

struct LocalStatus {
    int id_;
    int board_id_;
    QString name_;
    int position_;
    QString created_at_;
    QString updated_at_;
    QString deleted_at_;
    SyncStatus sync_status_;
    int server_version_;

    LocalStatus() = default;

    LocalStatus(int id, int board_id, const QString& name, int position,
                const QString& created_at = QString(), const QString& updated_at = QString(),
                const QString& deleted_at = QString(),
                const SyncStatus& sync_status = SyncStatus::PENDING, int server_version = 0)
        : id_(id)
        , board_id_(board_id)
        , name_(name)
        , position_(position)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , deleted_at_(deleted_at)
        , sync_status_(sync_status)
        , server_version_(server_version) {
    }
};
#endif // LOCAL_STATUS_HPP
