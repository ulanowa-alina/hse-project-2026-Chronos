#ifndef TASK_WINDOW_H
#define TASK_WINDOW_H

#include "network_manager.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QWidget>

class TaskCard : public QFrame {
    Q_OBJECT

  public:
    explicit TaskCard(int task_id, int board_id, int status_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onTaskSaveRequest();

  protected:
    void mousePressEvent(QMouseEvent* event) override;

  private:
    int task_id_;
    int board_id_;
    int status_id_;

    NetworkManager* network_manager_{nullptr};

    QLineEdit* title_{nullptr};
    QTextEdit* description_edit_{nullptr};

    void setupLayout();
};

#endif // TASK_WINDOW_H
