#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LoginScreen : public QWidget {
    Q_OBJECT

  public:
    explicit LoginScreen(QWidget* parent = nullptr);
  signals:
    void loginRequested();
    //void registrationRequested();

  private:
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
