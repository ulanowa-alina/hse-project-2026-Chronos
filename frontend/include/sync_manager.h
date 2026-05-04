#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include <QSqlDatabase>
#include "network_manager.h"
#include <QJsonArray>

class SyncManager{
  public:
    explicit SyncManager(QSqlDatabase& db, NetworkManager* manager);

    void syncBoards();
    void syncTasks();
    void syncStatuses();
    void syncAll();



    void parsingBoards(const QJsonArray& boards);

    //TODO: пока ручек get_all для Task и Status нет
    void parsingTasks(const QJsonArray& tasks);
    void parsingSatuses(const QJsonArray& statuses);


    void loadBoards();
    void loadTasks();
    void loadStatuses();
    void loadAll();


  private:
    QSqlDatabase db_;
    NetworkManager* network_manager_;

};
#endif //SYNC_MANAGER_H
