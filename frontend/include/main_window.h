#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "board_create_screen.h"
#include "board_edit_screen.h"
#include "board_screen.h"
#include "dashboard_screen.hpp"
#include "../sync/sync_coordinator.hpp"
#include "local_db.h"
#include "login_screen.h"
#include "network_manager.h"
#include "pomodoro_screen.h"
#include "profile_edit_screen.h"
#include "profile_screen.h"
#include "registration_screen.h"
#include "task_create_screen.h"
#include "task_edit_screen.h"
#include "welcome_screen.hpp"

#include <QMainWindow>
#include <QPointer>
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
    void switchToPomodoro();
    void switchToDashboard();
    void switchToTaskCreate(int board_id, int status_id);
    void switchToTaskEdit(int task_id, int board_id, int status_id);
    void switchToBoardCreate();
    void switchToBoardEdit(int board_id);
    void openLoginScreen();
    void onProfileLogout();
    void onProfileOpenBoard();
    void onProfileOpenDashboard();
    void onProfileEditBack();
    void onTaskCreateClose();
    void onTaskCreateDone();
    void onTaskEditClose();
    void onTaskEditDone();
    void onBoardCreateClose();
    void onBoardCreateDone();
    void onBoardEditClose();
    void onBoardEditDone();
    void onLoginSuccess();
    void onRegistrationSuccess();
    void onInitialDataReady(int board_id);
    void onDataChanged();
    void onAuthenticated(const QString& token);

  private:
    void restoreSession();
    void clearSession();

    NetworkManager* network_manager_;
    SyncCoordinator* sync_coordinator_;
    LocalDatabaseManager* local_db_{nullptr};
    QSqlDatabase db_;

    QStackedWidget* stacked_widget_{nullptr};
    WelcomeScreen* welcome_screen_{nullptr};
    BoardScreen* board_screen_{nullptr};
    DashboardScreen* dashboard_screen_{nullptr};

    QPointer<LoginScreen> login_screen_;
    QPointer<RegistrationScreen> registration_screen_;
    QPointer<ProfileScreen> profile_screen_;
    QPointer<ProfileEditScreen> profile_edit_screen_;
    QPointer<PomodoroScreen> pomodoro_screen_;
    QPointer<TaskCreateScreen> task_create_screen_;
    QPointer<TaskEditScreen> task_edit_screen_;
    QPointer<BoardCreateScreen> board_create_screen_;
    QPointer<BoardEditScreen> board_edit_screen_;

    int current_board_id_ = -1;

    void closeAllSmallWindows();
};
#endif // MAIN_WINDOW_H
