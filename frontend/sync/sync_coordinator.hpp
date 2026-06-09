#ifndef SYNC_COORDINATOR_HPP
#define SYNC_COORDINATOR_HPP

#include "board_sync_manager.hpp"
#include "network_manager.h"
#include "status_sync_manager.hpp"
#include "sync_manager.hpp"
#include "task_sync_manager.hpp"
#include "user_sync_manager.hpp"

#include <QJsonObject>
#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <memory>
#include <vector>

class SyncCoordinator : public QObject {
    Q_OBJECT

  public:
    SyncCoordinator(QSqlDatabase& db, NetworkManager* network_manager, QObject* parent = nullptr);

    void beginUserSession(const QJsonObject& user_obj);
    void loadCurrentUser();
    void clearLocalData();
    void loadAll(bool emit_initial_data = false);
    void syncAll();
    void syncBoards();
    void syncStatuses();
    void syncTasks();
    void syncUsers();
    void setPassword(const QString& password);
    void handleResponse(const QString& endpoint, const QByteArray& data, int code);
    void handleSyncResponse(const QString& endpoint, const QByteArray& data, int code,
                            const QString& entity, int local_id, const QString& operation);
    int currentUserId() const;
    int defaultBoardId() const;
    bool hasLocalData() const;
    void startPeriodicSync(int interval_ms = INTERVAL);
    void stopPeriodicSync();

  signals:
    void initialDataReady(int board_id);
    void dataChanged();

  private slots:
    void onPeriodicSync();

  private:
    QSqlDatabase& db_;
    NetworkManager* network_manager_;
    std::unique_ptr<UserSyncManager> user_manager_;
    std::unique_ptr<BoardSyncManager> board_manager_;
    std::unique_ptr<StatusSyncManager> status_manager_;
    std::unique_ptr<TaskSyncManager> task_manager_;
    std::vector<SyncManager*> managers_;
    QTimer* periodic_timer_{nullptr};
    bool waiting_initial_boards_{false};
    bool waiting_initial_statuses_{false};
    bool waiting_initial_tasks_{false};
    bool remote_loading_{false};
    bool emit_initial_after_load_{false};
    int current_user_id_{-1};

    static const int INTERVAL = 30'000;

    SyncManager* managerByModel(const QString& entity) const;
    void emitInitialData();
    void finishInitialLoadCycle();
    void loadChildrenAfterBoards();
    void loadTasksAfterStatuses();
};

#endif // SYNC_COORDINATOR_HPP
