#ifndef TASK_CARD_H
#define TASK_CARD_H

#include "network_manager.h"

#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPoint>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class TaskCard : public QFrame {
    Q_OBJECT

  public:
    explicit TaskCard(int task_id, int board_id, int status_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
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
                 const QDateTime& deadline = QDateTime(), bool is_completed = false);
    void updateTaskStatus();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onOpenSettings();
    void onTitleEditRequest();
    void onDeleteTaskRequest();
    void onMarkDoneRequest();
    void onUpdateTimer();

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    int task_id_;
    int board_id_;
    int status_id_;
    bool is_completed_{false};
    QPoint drag_start_position_;
    bool should_be_delete_{false};

    NetworkManager* network_manager_{nullptr};

    QDateTime deadline_;
    QTimer* timer_{nullptr};
    QLabel* deadline_label_{nullptr};

    QWidget* description_text_{nullptr};
    QLabel* description_label_{nullptr};
    QWidget* blue_line_{nullptr};

    QLineEdit* title_{nullptr};

    QPushButton* settings_button_{nullptr};
    QPushButton* complete_button_{nullptr};

    void setupLayout();
    void doneVisualState();
};

#endif // TASK_CARD_H
