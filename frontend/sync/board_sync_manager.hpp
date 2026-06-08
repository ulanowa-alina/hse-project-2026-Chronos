#ifndef BOARD_SYNC_MANAGER_HPP
#define BOARD_SYNC_MANAGER_HPP

#include "../local_repositories/local_board_repository.hpp"
#include "network_manager.h"
#include "sync_manager.hpp"

#include <QSqlDatabase>

class BoardSyncManager : public SyncManager {
  public:
    BoardSyncManager(QSqlDatabase& db, NetworkManager* network_manager);

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

    LocalBoard boardFromJson(const QJsonObject& obj) const;
    void saveFromServer(const LocalBoard& board);
};

#endif // BOARD_SYNC_MANAGER_HPP
