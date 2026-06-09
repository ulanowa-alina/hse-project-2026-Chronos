#ifndef TASK_CARD_H
#define TASK_CARD_H

#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"

#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QSqlDatabase>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class TaskCard : public QFrame {
    Q_OBJECT

  public:
    explicit TaskCard(int task_id, int board_id, int status_id, QSqlDatabase db,
                      QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    int getId() const {
        return task_id_;
    }
    int getTaskId() const {
        return task_id_;
    }
    void setStatusId(int new_status_id) {
        status_id_ = new_status_id;
    }
    void setData(const QString& title, const QString& description,
                 const QDateTime& deadline = QDateTime(), bool is_completed = false,
                 const QString& priority_color = QString());
    void updateTaskStatus();

  signals:
    void openTaskEditScreen(int task_id, int board_id, int status_id);

  private slots:

    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onTaskSaveRequest();
    void onOpenSettings();
    void onTitleEditRequest();
    void onDeleteTaskRequest();
    void onMarkDoneRequest();
    void onUpdateTimer();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    QSqlDatabase db_;

    int task_id_;
    int board_id_;
    int status_id_;
    bool is_completed_{false};
    bool should_be_delete_{false};

    QPoint drag_start_position_;

    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};

    QDateTime deadline_;
    QTimer* timer_{nullptr};
    QLabel* deadline_label_{nullptr};

    QWidget* description_text_{nullptr};
    QLabel* description_label_{nullptr};
    QWidget* blue_line_{nullptr};

    QLabel* drag_handle_{nullptr};
    QLineEdit* title_{nullptr};

    QPushButton* settings_button_{nullptr};
    QPushButton* complete_button_{nullptr};

    void startDrag();
    void scheduleDeletion();
    void setupLayout();
    void doneVisualState();
};

#endif // TASK_CARD_H
