#ifndef PROFILE_EDIT_SCREEN_H
#define PROFILE_EDIT_SCREEN_H

#include "network_manager.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class ProfileEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileEditScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void getUserData();

  signals:
    void profileRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onProfileEditRequest();

  private:
    NetworkManager* network_manager_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};

    QPushButton* save_button_{nullptr};
    QPushButton* cancel_button_{nullptr};

    QLineEdit* name_input_{nullptr};
    QLineEdit* email_input_{nullptr};
    QLineEdit* status_input_{nullptr};
    QLineEdit* password_input_{nullptr};

    void setupLayout();
};

#endif // PROFILE_EDIT_SCREEN_H
