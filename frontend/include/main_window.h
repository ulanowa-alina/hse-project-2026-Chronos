#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "board_screen.h"
#include "login_screen.h"
#include "network_manager.h"
#include "profile_screen.h"
#include "registration_screen.h"

#include <QMainWindow>
#include <QStackedWidget>
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow(QWidget* parent = nullptr);

  private slots:
    void switchToProfile();
    void switchToLogin();
    void switchToRegistration();
    void switchToBoard();

  private:
    NetworkManager* network_manager_;

    QStackedWidget* stacked_widget_{nullptr};
    LoginScreen* login_screen_{nullptr};
    ProfileScreen* profile_screen_{nullptr};
    RegistrationScreen* registration_screen_{nullptr};
    BoardScreen* board_screen_{nullptr};
};
#endif // MAIN_WINDOW_H
