#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H

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
    void addTaskCard(TaskCard* card);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onCreateTaskRequest();
    void onOpenSettings();
    void onStatusEditRequest();
    void onStatusDeleteRequest();

  private:
    int status_id_;
    int board_id_;
    bool should_be_delete_{false};
    bool should_be_highlighted_{false};

    NetworkManager* network_manager_{nullptr};

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
