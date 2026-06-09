#include "main_window.h"

#include <QDir>
#include <QSqlDatabase>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    network_manager_ = new NetworkManager(this);

    local_db_ = new LocalDatabaseManager(this);
    const QString db_path = QDir::current().absoluteFilePath("chronos_local.db");
    local_db_->open(db_path);
    local_db_->createSchema(QDir::current().filePath("../sql/local_init.sql"));

    db_ = local_db_->getDatabase();
    sync_coordinator_ = new SyncCoordinator(db_, network_manager_, this);

    stacked_widget_ = new QStackedWidget(this);
    setCentralWidget(stacked_widget_);

    welcome_screen_ = new WelcomeScreen(this);
    board_screen_ = new BoardScreen(current_board_id_, db_, this);
    dashboard_screen_ = new DashboardScreen(this);

    board_screen_->setNetworkManager(network_manager_);
    board_screen_->setSyncCoordinator(sync_coordinator_);
    dashboard_screen_->setNetworkManager(network_manager_);
    dashboard_screen_->setDatabase(db_);

    stacked_widget_->addWidget(welcome_screen_);
    stacked_widget_->addWidget(board_screen_);
    stacked_widget_->addWidget(dashboard_screen_);

    connect(welcome_screen_, &WelcomeScreen::loginRequested, this, &MainWindow::openLoginScreen);
    connect(board_screen_, &BoardScreen::openProfileScreen, this, &MainWindow::switchToProfile);
    connect(board_screen_, &BoardScreen::openPomodoroScreen, this, &MainWindow::switchToPomodoro);
    connect(board_screen_, &BoardScreen::openDashboardScreen, this, &MainWindow::switchToDashboard);
    connect(board_screen_, &BoardScreen::openTaskCreateScreen, this,
            &MainWindow::switchToTaskCreate);
    connect(board_screen_, &BoardScreen::openTaskEditScreen, this, &MainWindow::switchToTaskEdit);
    connect(board_screen_, &BoardScreen::openBoardEditScreen, this, &MainWindow::switchToBoardEdit);
    connect(dashboard_screen_, &DashboardScreen::openProfileScreen, this,
            &MainWindow::switchToProfile);
    connect(dashboard_screen_, &DashboardScreen::openBoardScreen, this, &MainWindow::switchToBoard);
    connect(dashboard_screen_, &DashboardScreen::openPomodoroScreen, this,
            &MainWindow::switchToPomodoro);
    connect(dashboard_screen_, &DashboardScreen::openBoardCreateScreen, this,
            &MainWindow::switchToBoardCreate);
    connect(dashboard_screen_, &DashboardScreen::logoutRequested, this, &MainWindow::switchToLogin);

    connect(network_manager_, &NetworkManager::responseReceived, sync_coordinator_,
            &SyncCoordinator::handleResponse);
    connect(network_manager_, &NetworkManager::syncResponseReceived, sync_coordinator_,
            &SyncCoordinator::handleSyncResponse);
    connect(sync_coordinator_, &SyncCoordinator::initialDataReady, this,
            &MainWindow::onInitialDataReady);
    connect(sync_coordinator_, &SyncCoordinator::dataChanged, this, &MainWindow::onDataChanged);

    setWindowTitle("Chronos");
    showFullScreen();
}

void MainWindow::closeAllSmallWindows() {
    if (login_screen_)
        login_screen_->close();
    if (registration_screen_)
        registration_screen_->close();
    if (profile_screen_)
        profile_screen_->close();
    if (profile_edit_screen_)
        profile_edit_screen_->close();
    if (pomodoro_screen_)
        pomodoro_screen_->close();
    if (task_create_screen_)
        task_create_screen_->close();
    if (task_edit_screen_)
        task_edit_screen_->close();
    if (board_create_screen_)
        board_create_screen_->close();
    if (board_edit_screen_)
        board_edit_screen_->close();
}

void MainWindow::restoreSession() {
    QSettings settings;
    QString token = settings.value("auth/token").toString();

    if (token.isEmpty()) {
        return;
    }

    network_manager_->setToken(token);

    if (!sync_coordinator_->hasLocalData()) {
        settings.remove("auth/token");
        network_manager_->clearToken();
        return;
    }

    sync_coordinator_->loadCurrentUser();

    const int board_id = sync_coordinator_->defaultBoardId();
    if (board_id > 0) {
        current_board_id_ = board_id;
    }

    sync_coordinator_->loadAll();
    sync_coordinator_->startPeriodicSync();

    switchToDashboard();
}

void MainWindow::onAuthenticated(const QString& token) {
    QSettings settings;
    settings.setValue("auth/token", token);
    sync_coordinator_->startPeriodicSync();
}

void MainWindow::clearSession() {
    QSettings settings;
    settings.remove("auth/token");
    sync_coordinator_->stopPeriodicSync();
    sync_coordinator_->clearLocalData();
}

void MainWindow::onInitialDataReady(int board_id) {
    if (board_id <= 0) {
        return;
    }

    current_board_id_ = board_id;
    switchToDashboard();
}

void MainWindow::onDataChanged() {
    if (stacked_widget_->currentWidget() == board_screen_) {
        board_screen_->reloadBoardData();
    }
}

void MainWindow::openLoginScreen() {
    closeAllSmallWindows();
    if (!login_screen_) {
        login_screen_ = new LoginScreen();
        login_screen_->setNetworkManager(network_manager_);
        login_screen_->setSyncCoordinator(sync_coordinator_);
        connect(login_screen_, &LoginScreen::registrationRequested, this,
                &MainWindow::switchToRegistration);
        connect(login_screen_, &LoginScreen::authenticated, this, &MainWindow::onAuthenticated);
        connect(login_screen_, &LoginScreen::authenticated, this, &MainWindow::onLoginSuccess);
    }
    login_screen_->clearInputs();
    login_screen_->show();
    login_screen_->raise();
    login_screen_->activateWindow();
    login_screen_->resize(420, 620);
    setWindowTitle("Chronos - Вход");
}

void MainWindow::onLoginSuccess() {
    if (registration_screen_) {
        registration_screen_->close();
    }
    if (login_screen_) {
        login_screen_->close();
    }
}

void MainWindow::switchToRegistration() {
    if (login_screen_)
        login_screen_->close();
    if (!registration_screen_) {
        registration_screen_ = new RegistrationScreen();
        registration_screen_->setNetworkManager(network_manager_);
        registration_screen_->setSyncCoordinator(sync_coordinator_);
        connect(registration_screen_, &RegistrationScreen::loginRequested, this,
                &MainWindow::openLoginScreen);
        connect(registration_screen_, &RegistrationScreen::registrationRequested, this,
                &MainWindow::onRegistrationSuccess);
        connect(registration_screen_, &RegistrationScreen::authenticated, this,
                &MainWindow::onAuthenticated);
    }
    registration_screen_->clearInputs();
    registration_screen_->show();
    registration_screen_->raise();
    registration_screen_->activateWindow();
    registration_screen_->resize(420, 620);
    setWindowTitle("Chronos - Регистрация");
}

void MainWindow::onRegistrationSuccess() {
    closeAllSmallWindows();
    switchToDashboard();
}

void MainWindow::switchToProfile() {
    closeAllSmallWindows();
    if (!profile_screen_) {
        profile_screen_ = new ProfileScreen();
        profile_screen_->setNetworkManager(network_manager_);
        profile_screen_->setDatabase(db_);
        profile_screen_->setSyncCoordinator(sync_coordinator_);
        connect(profile_screen_, &ProfileScreen::logoutRequested, this,
                &MainWindow::onProfileLogout);
        connect(profile_screen_, &ProfileScreen::boardRequested, this,
                &MainWindow::onProfileOpenBoard);
        connect(profile_screen_, &ProfileScreen::openDashboardScreen, this,
                &MainWindow::onProfileOpenDashboard);
        connect(profile_screen_, &ProfileScreen::profileEditRequested, this,
                &MainWindow::switchToProfileEdit);
    }
    profile_screen_->reloadFromLocal();
    profile_screen_->show();
    profile_screen_->raise();
    profile_screen_->activateWindow();
    profile_screen_->resize(420, 620);
    setWindowTitle("Chronos - Профиль");
}

void MainWindow::onProfileLogout() {
    switchToLogin();
}

void MainWindow::onProfileOpenBoard() {
    closeAllSmallWindows();
    switchToBoard(current_board_id_);
}

void MainWindow::onProfileOpenDashboard() {
    closeAllSmallWindows();
    switchToDashboard();
    showFullScreen();
}

void MainWindow::switchToProfileEdit() {
    if (profile_screen_)
        profile_screen_->close();
    if (!profile_edit_screen_) {
        profile_edit_screen_ = new ProfileEditScreen();
        profile_edit_screen_->setNetworkManager(network_manager_);
        profile_edit_screen_->setDatabase(db_);
        profile_edit_screen_->setSyncCoordinator(sync_coordinator_);
        connect(profile_edit_screen_, &ProfileEditScreen::profileRequested, this,
                &MainWindow::onProfileEditBack);
    }
    profile_edit_screen_->reloadFromLocal();
    profile_edit_screen_->show();
    profile_edit_screen_->raise();
    profile_edit_screen_->activateWindow();
    profile_edit_screen_->resize(420, 620);
    setWindowTitle("Chronos - Редактирование");
}

void MainWindow::onProfileEditBack() {
    switchToProfile();
}
void MainWindow::switchToLogin() {
    clearSession();
    network_manager_->clearToken();
    current_board_id_ = -1;
    board_screen_->setId(-1);
    board_screen_->clearBoardData();
    closeAllSmallWindows();

    stacked_widget_->setCurrentWidget(welcome_screen_);
    setWindowTitle("Chronos");
    showFullScreen();
}

void MainWindow::switchToBoard(int board_id) {
    if (board_id > 0) {
        current_board_id_ = board_id;
    }

    board_screen_->setId(current_board_id_);
    board_screen_->reloadBoardData();

    stacked_widget_->setCurrentWidget(board_screen_);
    setWindowTitle("Chronos - Доска");
    showFullScreen();
}

void MainWindow::switchToPomodoro() {
    closeAllSmallWindows();
    if (!pomodoro_screen_) {
        pomodoro_screen_ = new PomodoroScreen();
        pomodoro_screen_->setNetworkManager(network_manager_);
        connect(pomodoro_screen_, &PomodoroScreen::openProfileScreen, this,
                &MainWindow::switchToProfile);
    }
    pomodoro_screen_->show();
    pomodoro_screen_->raise();
    pomodoro_screen_->activateWindow();
    pomodoro_screen_->resize(420, 620);
    setWindowTitle("Chronos - Pomodoro");
}

void MainWindow::switchToDashboard() {
    closeAllSmallWindows();
    stacked_widget_->setCurrentWidget(dashboard_screen_);
    dashboard_screen_->reloadDashboardData();
    setWindowTitle("Chronos - Dashboard");
    showFullScreen();
}

void MainWindow::switchToTaskCreate(int board_id, int status_id) {
    closeAllSmallWindows();
    if (!task_create_screen_) {
        task_create_screen_ = new TaskCreateScreen(board_id, status_id);
        task_create_screen_->setNetworkManager(network_manager_);
        task_create_screen_->setDatabase(&db_);
        task_create_screen_->setSyncCoordinator(sync_coordinator_);
        connect(task_create_screen_, &TaskCreateScreen::closeRequested, this,
                &MainWindow::onTaskCreateClose);
        connect(task_create_screen_, &TaskCreateScreen::taskCreated, this,
                &MainWindow::onTaskCreateDone);
    }
    task_create_screen_->setBoardId(board_id);
    task_create_screen_->setStatusId(status_id);
    task_create_screen_->show();
    task_create_screen_->raise();
    task_create_screen_->activateWindow();
    task_create_screen_->resize(420, 620);
    setWindowTitle("Chronos - Создать задачу");
}

void MainWindow::onTaskCreateClose() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}

void MainWindow::onTaskCreateDone() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}

void MainWindow::switchToTaskEdit(int task_id, int board_id, int status_id) {
    closeAllSmallWindows();
    if (!task_edit_screen_) {
        task_edit_screen_ = new TaskEditScreen(task_id, board_id, status_id);
        task_edit_screen_->setNetworkManager(network_manager_);
        task_edit_screen_->setSyncCoordinator(sync_coordinator_);
        task_edit_screen_->setDatabase(db_);
        connect(task_edit_screen_, &TaskEditScreen::closeRequested, this,
                &MainWindow::onTaskEditClose);
        connect(task_edit_screen_, &TaskEditScreen::taskUpdated, this, &MainWindow::onTaskEditDone);
    }
    task_edit_screen_->setTaskId(task_id);
    task_edit_screen_->setBoardId(board_id);
    task_edit_screen_->setStatusId(status_id);
    task_edit_screen_->loadTaskData();
    task_edit_screen_->show();
    task_edit_screen_->raise();
    task_edit_screen_->activateWindow();
    task_edit_screen_->resize(420, 620);
    setWindowTitle("Chronos - Редактировать задачу");
}

void MainWindow::onTaskEditClose() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}

void MainWindow::onTaskEditDone() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}

void MainWindow::switchToBoardCreate() {
    closeAllSmallWindows();
    if (!board_create_screen_) {
        board_create_screen_ = new BoardCreateScreen();
        board_create_screen_->setNetworkManager(network_manager_);
        board_create_screen_->setDatabase(&db_);
        board_create_screen_->setSyncCoordinator(sync_coordinator_);
        board_create_screen_->setUserId(sync_coordinator_->currentUserId());
        connect(board_create_screen_, &BoardCreateScreen::closeRequested, this,
                &MainWindow::onBoardCreateClose);
        connect(board_create_screen_, &BoardCreateScreen::boardCreated, this,
                &MainWindow::onBoardCreateDone);
    }
    board_create_screen_->show();
    board_create_screen_->raise();
    board_create_screen_->activateWindow();
    board_create_screen_->resize(420, 620);
    setWindowTitle("Chronos - Создать доску");
}

void MainWindow::onBoardCreateClose() {
    closeAllSmallWindows();
    switchToDashboard();
}

void MainWindow::onBoardCreateDone() {
    closeAllSmallWindows();
    switchToDashboard();
}

void MainWindow::switchToBoardEdit(int board_id) {
    closeAllSmallWindows();
    if (!board_edit_screen_) {
        board_edit_screen_ = new BoardEditScreen(board_id);
        board_edit_screen_->setNetworkManager(network_manager_);
        board_edit_screen_->setSyncCoordinator(sync_coordinator_);
        board_edit_screen_->setDatabase(db_);
        connect(board_edit_screen_, &BoardEditScreen::closeRequested, this,
                &MainWindow::onBoardEditClose);
        connect(board_edit_screen_, &BoardEditScreen::boardUpdated, this,
                &MainWindow::onBoardEditDone);
    }
    board_edit_screen_->setBoardId(board_id);
    board_edit_screen_->loadBoardData();
    board_edit_screen_->show();
    board_edit_screen_->raise();
    board_edit_screen_->activateWindow();
    board_edit_screen_->resize(420, 620);
    setWindowTitle("Chronos - Редактировать доску");
}

void MainWindow::onBoardEditClose() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}

void MainWindow::onBoardEditDone() {
    closeAllSmallWindows();
    if (current_board_id_ != -1) {
        switchToBoard(current_board_id_);
    } else {
        switchToDashboard();
    }
}
