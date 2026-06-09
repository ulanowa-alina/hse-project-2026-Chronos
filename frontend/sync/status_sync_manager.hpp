#ifndef STATUS_SYNC_MANAGER_HPP
#define STATUS_SYNC_MANAGER_HPP

#include "../local_repositories/local_status_repository.hpp"
#include "network_manager.h"
#include "sync_manager.hpp"

#include <QSqlDatabase>

class StatusSyncManager : public SyncManager {
  public:
    StatusSyncManager(QSqlDatabase& db, NetworkManager* network_manager);

    void sync() override;
    void parse(const QJsonArray& data) override;
    void parseObject(const QJsonObject& data) override;
    void load() override;

    QString modelName() const override;
    QString loadEndpoint() const override;
    bool handlesLoadEndpoint(const QString& endpoint) const override;
    bool handlePushResponse(const QString& endpoint, const QByteArray& data, int code, int local_id,
                            const QString& operation) override;

  private:
    QSqlDatabase& db_;
    NetworkManager* network_manager_;

    LocalStatus statusFromJson(const QJsonObject& obj) const;
    void saveFromServer(const LocalStatus& status);
    bool isParentBoardSynced(int board_id) const;
};

#endif // STATUS_SYNC_MANAGER_HPP
