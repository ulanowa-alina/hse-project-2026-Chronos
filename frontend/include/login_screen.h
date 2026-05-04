#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include "sync_manager.h"

class LoginScreen : public QWidget {
    Q_OBJECT

  public:
    explicit LoginScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncManager(SyncManager* manager);

  signals:
    void loginRequested(int board_id);
    void registrationRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onLoginRequest();

  private:
    NetworkManager* network_manager_{nullptr};
    SyncManager* sync_manager_{nullptr};


    QPushButton* login_button_{nullptr};
    QPushButton* registration_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* no_account_label_{nullptr};
    QLabel* login_title_label_{nullptr};

    QLineEdit* email_input_{nullptr};
    QLineEdit* password_input_{nullptr};

    void setupLayout();
};
#endif // LOGIN_SCREEN_H
