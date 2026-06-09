#ifndef LOCAL_BOARD_HPP
#define LOCAL_BOARD_HPP

#include "sync_status.hpp"

#include <QString>

struct LocalBoard {
    int id_;
    int user_id_;
    QString title_;
    QString description_;
    QString created_at_;
    QString updated_at_;
    QString deleted_at_;
    SyncStatus sync_status_;
    int server_version_;

    LocalBoard() = default;

    LocalBoard(int id, int user_id, const QString& title, const QString desc,
               const QString& created_at = QString(), const QString& updated_at = QString(),
               const QString& deleted_at = QString(),
               const SyncStatus& sync_status = SyncStatus::PENDING, int server_version = 0)
        : id_(id)
        , user_id_(user_id)
        , title_(title)
        , description_(desc)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , deleted_at_(deleted_at)
        , sync_status_(sync_status)
        , server_version_(server_version) {
    }
};
#endif // LOCAL_BOARD_HPP
