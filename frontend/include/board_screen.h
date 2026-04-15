#ifndef BOARD_SCREEN_H
#define BOARD_SCREEN_H

#include "network_manager.h"
#include "status_window.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class BoardScreen : public QWidget {
    Q_OBJECT

  public:
    explicit BoardScreen(int board_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void reloadBoardData();
    void clearBoardData();
    void setId(int id) {
        board_id_ = id;
    }

  signals:
    void openProfileScreen();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onStatusCreateRequest();
    void onProfileRequest();

  private:
    int board_id_;

    NetworkManager* network_manager_{nullptr};

    QPushButton* profile_button_{nullptr};
    QPushButton* status_create_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* board_name_label_{nullptr};

    QScrollArea* scroll_area_{nullptr};
    QHBoxLayout* board_layout_{nullptr};
    QMap<int, StatusWindow*> status_windows_;
    QMap<int, QString> status_names_;

    StatusWindow* ensureStatusWindow(int status_id, const QString& name);
    void loadTasksFromResponse(const QByteArray& data);
    void setupLayout();
};

#endif // BOARD_SCREEN_H
