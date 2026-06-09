#ifndef BOARD_SCREEN_H
#define BOARD_SCREEN_H

#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"
#include "status_window.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSqlDatabase>
#include <QVBoxLayout>
#include <QWidget>

class BoardScreen : public QWidget {
    Q_OBJECT

  public:
    explicit BoardScreen(int board_id, QSqlDatabase db, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void reloadBoardData();
    void clearBoardData();
    void setId(int id) {
        board_id_ = id;
    }

  signals:
    void openProfileScreen();
    void openPomodoroScreen();
    void openTaskCreateScreen(int board_id, int status_id);
    void openDashboardScreen();
    void openTaskEditScreen(int task_id, int board_id, int status_id);
    void openBoardEditScreen(int board_id);

  private slots:
    void onStatusCreateRequest();
    void onProfileRequest();
    void onAvatarImageDownloaded(QNetworkReply* reply);
    void onPomodoroRequest();
    void onBoardSettingsRequested();
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

  private:
    int board_id_;
    QSqlDatabase db_;

    NetworkManager* network_manager_{nullptr};
    QNetworkAccessManager* avatar_network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};

    QPushButton* profile_button_{nullptr};
    QPushButton* status_create_button_{nullptr};
    QPushButton* pomodoro_button_{nullptr};
    QPushButton* board_settings_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* board_name_label_{nullptr};

    QScrollArea* scroll_area_{nullptr};
    QHBoxLayout* board_layout_{nullptr};
    QMap<int, StatusWindow*> status_windows_;
    QMap<int, QString> status_names_;

    void loadAvatar(const QString& avatar_s3_key);
    void setDefaultAvatar();
    StatusWindow* showStatusWindow(int status_id, const QString& name);
    void loadFromLocalDatabase();
    void setupLayout();
};

#endif // BOARD_SCREEN_H
