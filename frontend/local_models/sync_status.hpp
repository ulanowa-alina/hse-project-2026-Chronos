#ifndef SYNC_STATUS_HPP
#define SYNC_STATUS_HPP

#include <QString>

enum class SyncStatus{
    PENDING,
    SYNCED,
    CONFLICT
};

inline QString syncStatusToString(const SyncStatus& sync_status){
    if (sync_status == SyncStatus::SYNCED){
        return "synced";
    }
    if(sync_status == SyncStatus::CONFLICT){
        return "conflict";
    }
    return "pending";
}

inline SyncStatus stringToSyncStatus(const QString& s){
    if(s == "synced"){
        return SyncStatus::SYNCED;
    }
    if(s == "conflict"){
        return SyncStatus::CONFLICT;
    }
    return SyncStatus::PENDING;
}
#endif // SYNC_STATUS_HPP
