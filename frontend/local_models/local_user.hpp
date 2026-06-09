#ifndef LOCAL_USER_HPP
#define LOCAL_USER_HPP

#include "sync_status.hpp"

#include <QString>

struct LocalUser {
    int id_;
    QString email_;
    QString name_;
    QString status_;
    QString password_hash_;
    QString created_at_;
    QString updated_at_;
    QString deleted_at_;
    SyncStatus sync_status_;
    int server_version_;

    LocalUser() = default;

    LocalUser(int id, const QString& email, const QString& name, const QString& status,
              const QString& password_hash = QString(), const QString& created_at = QString(),
              const QString& updated_at = QString(), const QString& deleted_at = QString(),
              const SyncStatus& sync_status = SyncStatus::PENDING, int server_version = 0)
        : id_(id)
        , email_(email)
        , name_(name)
        , status_(status)
        , password_hash_(password_hash)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , deleted_at_(deleted_at)
        , sync_status_(sync_status)
        , server_version_(server_version) {
    }
};
#endif // LOCAL_USER_HPP
