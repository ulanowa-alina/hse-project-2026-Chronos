#ifndef TASK_WINDOW_H
#define TASK_WINDOW_H

#include "../sync/sync_coordinator.hpp"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QSqlDatabase>
#include <QStackedWidget>
#include <QTextEdit>
#include <QWidget>

class TaskCard : public QFrame {
    Q_OBJECT

  public:
    explicit TaskCard(int task_id, int board_id, int status_id, QSqlDatabase db,
                      QWidget* parent = nullptr);

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
    void setData(const QString& title, const QString& description);
    void updateTaskStatus();

  private slots:
    void onTaskSaveRequest();
    void onOpenSettings();
    void onTitleEditRequest();
    void onDescriptionEditRequest();
    void onDeleteTaskRequest();

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    int task_id_;
    int board_id_;
    int status_id_;
    QSqlDatabase db_;
    QPoint drag_start_position_;

    SyncCoordinator* sync_coordinator_{nullptr};

    QLineEdit* title_{nullptr};
    QTextEdit* description_edit_{nullptr};
    QPushButton* settings_button_{nullptr};

    void setupLayout();
};

#endif // TASK_WINDOW_H
