#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../local_repositories/local_task_repository.hpp"
#include "board_screen.h"
#include "local_db.h"
#include "login_screen.h"
#include "network_manager.h"
#include "profile_edit_screen.h"
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
    void switchToBoard(int board_id);
    void switchToProfileEdit();

  private:
    NetworkManager* network_manager_;
    LocalDatabaseManager* local_db_{nullptr};

    QStackedWidget* stacked_widget_{nullptr};
    LoginScreen* login_screen_{nullptr};
    ProfileScreen* profile_screen_{nullptr};
    RegistrationScreen* registration_screen_{nullptr};
    BoardScreen* board_screen_{nullptr};
    ProfileEditScreen* profile_edit_screen_{nullptr};

    int current_board_id_ = -1;
};
#endif // MAIN_WINDOW_H
