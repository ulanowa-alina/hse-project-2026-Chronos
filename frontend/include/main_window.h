#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../sync/sync_coordinator.hpp"
#include "board_screen.h"
#include "local_db.h"
#include "login_screen.h"
#include "network_manager.h"
#include "profile_edit_screen.h"
#include "profile_screen.h"
#include "registration_screen.h"

#include <QMainWindow>
#include <QSettings>
#include <QSqlDatabase>
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
    void onInitialDataReady(int board_id);
    void onDataChanged();

  private:
    void restoreSession();
    void saveToken(const QString& token);
    void clearSession();

    NetworkManager* network_manager_;
    SyncCoordinator* sync_coordinator_;
    LocalDatabaseManager* local_db_{nullptr};
    QSqlDatabase db_;

    QStackedWidget* stacked_widget_{nullptr};
    LoginScreen* login_screen_{nullptr};
    ProfileScreen* profile_screen_{nullptr};
    RegistrationScreen* registration_screen_{nullptr};
    BoardScreen* board_screen_{nullptr};
    ProfileEditScreen* profile_edit_screen_{nullptr};

    int current_board_id_ = -1;
};
#endif // MAIN_WINDOW_H
