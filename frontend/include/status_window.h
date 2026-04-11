#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H

#include "network_manager.h"
#include "task_card.h"

#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
class StatusWindow : public QFrame {
    Q_OBJECT

  public:
    explicit StatusWindow(int status_id, int board_id, const QString& name,
                          QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);

    int getId() {
        return status_id_;
    }
    int setId(int id) {
        return status_id_ = id;
    }

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onCreateTaskRequest();
    void onOpenSettings();
    void onStatusEditRequest();

  private:
    int status_id_;
    int board_id_;

    NetworkManager* network_manager_{nullptr};

    QLineEdit* status_name_{nullptr};

    QPushButton* create_task_button_{nullptr};
    QPushButton* settings_button_{nullptr};

    QScrollArea* tasks_scroll_area_{nullptr};
    QVBoxLayout* tasks_layout_{nullptr};

    void setupLayout(const QString& name);
};
#endif // STATUS_WINDOW_H
