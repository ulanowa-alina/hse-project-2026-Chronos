#ifndef REGISTRATION_SCREEN_H
#define REGISTRATION_SCREEN_H

#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class RegistrationScreen : public QWidget {
    Q_OBJECT

  public:
    explicit RegistrationScreen(QWidget* parent = nullptr);

    void set_network_manager(NetworkManager* manager);

  signals:
    void loginRequested();
    void registrationRequested();

  private slots:
    void on_network_response(const QString& endpoint, const QByteArray& data, int code);
    void on_register_request();

  private:
    NetworkManager* network_manager_{nullptr};

    QPushButton* login_button_{nullptr};
    QPushButton* registration_button_{nullptr};
    QPushButton* avatar_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* no_account_label_{nullptr};
    QLabel* login_title_label_{nullptr};

    QLineEdit* email_input_{nullptr};
    QLineEdit* password_input_{nullptr};
    QLineEdit* name_input_{nullptr};
    QLineEdit* status_input_{nullptr};

    void setupLayout();
};
#endif // REGISTRATION_SCREEN_H
