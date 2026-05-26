#include "local_user_repository.hpp"


#include <QSqlError>
#include <QSqlQuery>

LocalUser createUser(QSqlQuery& query){
    return LocalUser(query.value("id").toInt(), query.value("email").toString(),
                     query.value("name").toString(), query.value("status").toString(),
                     query.value("created_at").toString(), query.value("updated_at").toString(),
                     query.value("deleted_at").toString(), stringToSyncStatus(query.value("sync_status").toString()),
                     query.value("server_version").toInt());
}

LocalUserRepository::LocalUserRepository(QSqlDatabase& db)
    : db_(db){

}

LocalUser LocalUserRepository::insert(const LocalUser& user) {

}
