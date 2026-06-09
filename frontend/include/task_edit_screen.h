#ifndef TASK_EDIT_SCREEN_H
#define TASK_EDIT_SCREEN_H

#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class TaskEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit TaskEditScreen(int task_id, int board_id, int status_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void setDatabase(QSqlDatabase db);
    void setTaskId(int task_id);
    void setBoardId(int board_id);
    void setStatusId(int status_id);
    void loadTaskData();

  signals:
    void taskUpdated();
    void closeRequested();

  private slots:
    void onUpdateTaskRequest();
    void onCloseRequest();
    void onDeadlineCheckChanged(Qt::CheckState state);

  private:
    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};
    QSqlDatabase db_;
    int task_id_{-1};
    int board_id_{-1};
    int status_id_{-1};

    QPushButton* update_button_{nullptr};
    QPushButton* cancel_button_{nullptr};
    QPushButton* close_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* title_label_{nullptr};

    QLineEdit* title_input_{nullptr};
    QTextEdit* description_input_{nullptr};
    QComboBox* priority_combo_{nullptr};
    QDateTimeEdit* deadline_input_{nullptr};
    QCheckBox* deadline_checkbox_{nullptr};

    void setupLayout();
};

#endif // TASK_EDIT_SCREEN_H
