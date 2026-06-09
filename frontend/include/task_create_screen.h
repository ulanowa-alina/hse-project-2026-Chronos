#ifndef TASK_CREATE_SCREEN_H
#define TASK_CREATE_SCREEN_H

#include "../local_repositories/local_task_repository.hpp"
#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class TaskCreateScreen : public QWidget {
    Q_OBJECT

  public:
    explicit TaskCreateScreen(int board_id, int status_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void setBoardId(int board_id);
    void setStatusId(int status_id);
    void setDatabase(QSqlDatabase* db);

  signals:
    void taskCreated();
    void closeRequested();

  private slots:
    void onCreateTaskRequest();
    void onCloseRequest();
    void onDeadlineCheckChanged(Qt::CheckState state);

  private:
    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};
    QSqlDatabase* db_{nullptr};
    int board_id_{-1};
    int status_id_{-1};

    QPushButton* create_button_{nullptr};
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
    void clearFields();
};

#endif // TASK_CREATE_SCREEN_H
