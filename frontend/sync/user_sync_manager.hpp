#ifndef USER_SYNC_MANAGER_HPP
#define USER_SYNC_MANAGER_HPP

#include "../local_repositories/local_user_repository.hpp"
#include "network_manager.h"
#include "sync_manager.hpp"

#include <QSqlDatabase>

class UserSyncManager : public SyncManager {
  public:
    UserSyncManager(QSqlDatabase& db, NetworkManager* network_manager);

    void sync() override;
    void parse(const QJsonArray& data) override;
    void parseObject(const QJsonObject& data) override;
    void load() override;

    void saveFromLogin(const QJsonObject& user_obj);
    void setPassword(const QString& password);

    QString modelName() const override;
    QString loadEndpoint() const override;
    bool handlesLoadEndpoint(const QString& endpoint) const override;
    bool handlePushResponse(const QString& endpoint, const QByteArray& data, int code, int local_id,
                            const QString& operation) override;

  private:
    QSqlDatabase& db_;
    NetworkManager* network_manager_;
    QString pending_password_;

    LocalUser userFromJson(const QJsonObject& obj) const;
    void saveFromServer(const LocalUser& user);
};

#endif // USER_SYNC_MANAGER_HPP
