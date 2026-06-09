#ifndef TASK_SYNC_MANAGER_HPP
#define TASK_SYNC_MANAGER_HPP

#include "../local_repositories/local_task_repository.hpp"
#include "network_manager.h"
#include "sync_manager.hpp"

#include <QSqlDatabase>

class TaskSyncManager : public SyncManager {
  public:
    TaskSyncManager(QSqlDatabase& db, NetworkManager* network_manager);

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

    LocalTask taskFromJson(const QJsonObject& obj) const;
    void saveFromServer(const LocalTask& task);
    bool isParentBoardSynced(int board_id) const;
};

#endif // TASK_SYNC_MANAGER_HPP
