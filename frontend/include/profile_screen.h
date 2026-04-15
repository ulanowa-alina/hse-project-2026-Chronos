#ifndef PROFILE_SCREEN_H
#define PROFILE_SCREEN_H

#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWidget>

class ProfileScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void getUserData();

  signals:
    void logoutRequested();
    void boardRequested();
    void profileEditRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onProfileEditRequest();
  private:
    NetworkManager* network_manager_{nullptr};

    QPushButton* edit_button_{nullptr};
    QPushButton* logout_button_{nullptr};
    QPushButton* logo_button_{nullptr};

    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};
    QLabel* status_label_{nullptr};
    QLabel* email_label_{nullptr};

    void setupLayout();
};

#endif // PROFILE_SCREEN_H
