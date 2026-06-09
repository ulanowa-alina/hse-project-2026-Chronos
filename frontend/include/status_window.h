#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H

#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"
#include "task_card.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlDatabase>
#include <QVBoxLayout>
#include <QWidget>

class StatusWindow : public QFrame {
    Q_OBJECT

  public:
    explicit StatusWindow(int status_id, int board_id, const QString& name, QSqlDatabase db,
                          QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);

    int getId() {
        return status_id_;
    }
    int setId(int id) {
        return status_id_ = id;
    }
    void addTaskCard(TaskCard* card);
    void clearTasks();

  signals:
    void openTaskCreateScreen(int board_id, int status_id);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

  private slots:
    void onCreateTaskRequest();
    void onOpenSettings();
    void onStatusEditRequest();
    void onStatusDeleteRequest();
    void onStatusNameSaved();
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);

  private:
    int status_id_;
    int board_id_;
    QSqlDatabase db_;
    bool should_be_highlighted_{false};

    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};

    QLineEdit* status_name_{nullptr};

    QPushButton* create_task_button_{nullptr};
    QPushButton* settings_button_{nullptr};

    QScrollArea* tasks_scroll_area_{nullptr};
    QWidget* tasks_container_{nullptr};
    QVBoxLayout* tasks_layout_{nullptr};

    void insertTaskCard(TaskCard* card);
    void removeTaskCard(TaskCard* card);
    void processHighlight();
    void setupLayout(const QString& name);
};
#endif // STATUS_WINDOW_H
