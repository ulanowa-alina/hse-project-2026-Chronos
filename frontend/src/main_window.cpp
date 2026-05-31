#include "main_window.h"

#include <QSqlDatabase>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    network_manager_ = new NetworkManager(this);

    local_db_ = new LocalDatabaseManager(this);
    local_db_->open("chronos_local.db");
    local_db_->createSchema("../sql/local_init.sql");

    db_ = local_db_->getDatabase();
    sync_coordinator_ = new SyncCoordinator(db_, network_manager_, this);

    stacked_widget_ = new QStackedWidget(this);
    setCentralWidget(stacked_widget_);

    login_screen_ = new LoginScreen(this);
    profile_screen_ = new ProfileScreen(this);
    profile_edit_screen_ = new ProfileEditScreen(this);
    registration_screen_ = new RegistrationScreen(this);
    board_screen_ = new BoardScreen(current_board_id_, db_, this);

    login_screen_->setNetworkManager(network_manager_);
    login_screen_->setSyncCoordinator(sync_coordinator_);
    profile_screen_->setDatabase(db_);
    profile_screen_->setSyncCoordinator(sync_coordinator_);
    profile_edit_screen_->setDatabase(db_);
    profile_edit_screen_->setSyncCoordinator(sync_coordinator_);
    registration_screen_->setNetworkManager(network_manager_);
    registration_screen_->setSyncCoordinator(sync_coordinator_);
    board_screen_->setNetworkManager(network_manager_);
    board_screen_->setSyncCoordinator(sync_coordinator_);

    stacked_widget_->addWidget(login_screen_);
    stacked_widget_->addWidget(registration_screen_);
    stacked_widget_->addWidget(board_screen_);
    stacked_widget_->addWidget(profile_screen_);
    stacked_widget_->addWidget(profile_edit_screen_);

    connect(login_screen_, &LoginScreen::loginRequested, this, &MainWindow::switchToBoard);
    connect(profile_screen_, &ProfileScreen::logoutRequested, this, &MainWindow::switchToLogin);
    connect(profile_screen_, &ProfileScreen::boardRequested, this,
            [this]() { this->switchToBoard(this->current_board_id_); });
    connect(registration_screen_, &RegistrationScreen::loginRequested, this,
            &MainWindow::switchToLogin);
    connect(registration_screen_, &RegistrationScreen::registrationRequested, this,
            &MainWindow::switchToBoard);
    connect(login_screen_, &LoginScreen::registrationRequested, this,
            &MainWindow::switchToRegistration);
    connect(board_screen_, &BoardScreen::openProfileScreen, this, &MainWindow::switchToProfile);
    connect(profile_screen_, &ProfileScreen::profileEditRequested, this,
            &MainWindow::switchToProfileEdit);
    connect(profile_edit_screen_, &ProfileEditScreen::profileRequested, this,
            &MainWindow::switchToProfile);

    connect(network_manager_, &NetworkManager::responseReceived, sync_coordinator_,
            &SyncCoordinator::handleResponse);
    connect(network_manager_, &NetworkManager::syncResponseReceived, sync_coordinator_,
            &SyncCoordinator::handleSyncResponse);
    connect(sync_coordinator_, &SyncCoordinator::initialDataReady, this,
            &MainWindow::onInitialDataReady);
    connect(sync_coordinator_, &SyncCoordinator::dataChanged, this, &MainWindow::onDataChanged);

    connect(login_screen_, &LoginScreen::authenticated, this, &MainWindow::saveToken);
    connect(registration_screen_, &RegistrationScreen::authenticated, this, &MainWindow::saveToken);

    setWindowTitle("Chronos");
    resize(400, 600);

    restoreSession();
}

void MainWindow::restoreSession() {
    QSettings settings;
    const QString token = settings.value("auth/token").toString();
    if (!token.isEmpty()) {
        network_manager_->setToken(token);
    }

    if (sync_coordinator_->hasLocalData()) {
        const int board_id = sync_coordinator_->defaultBoardId();
        if (board_id > 0) {
            switchToBoard(board_id);
        }
    }

    if (network_manager_->hasToken()) {
        sync_coordinator_->loadAll();
        sync_coordinator_->startPeriodicSync();
    }
}

void MainWindow::saveToken(const QString& token) {
    QSettings settings;
    settings.setValue("auth/token", token);
    sync_coordinator_->startPeriodicSync();
}

void MainWindow::clearSession() {
    QSettings settings;
    settings.remove("auth/token");
    sync_coordinator_->stopPeriodicSync();
}

void MainWindow::onInitialDataReady(int board_id) {
    if (board_id > 0 && stacked_widget_->currentWidget() != board_screen_) {
        switchToBoard(board_id);
    } else if (board_id > 0) {
        board_screen_->setId(board_id);
        board_screen_->reloadBoardData();
    }
}

void MainWindow::onDataChanged() {
    if (stacked_widget_->currentWidget() == board_screen_) {
        board_screen_->reloadBoardData();
    }
    if (stacked_widget_->currentWidget() == profile_screen_) {
        profile_screen_->reloadFromLocal();
    }
}

void MainWindow::switchToProfile() {
    stacked_widget_->setCurrentWidget(profile_screen_);
    profile_screen_->reloadFromLocal();
    setWindowTitle("Chronos - Профиль");

    if (isMaximized()) {
        showNormal();
    }
    resize(420, 620);
}

void MainWindow::switchToProfileEdit() {
    stacked_widget_->setCurrentWidget(profile_edit_screen_);
    profile_edit_screen_->reloadFromLocal();
    setWindowTitle("Chronos - Редактирование");

    if (isMaximized()) {
        showNormal();
    }
    resize(420, 620);
}

void MainWindow::switchToLogin() {
    clearSession();
    network_manager_->clearToken();
    current_board_id_ = -1;
    board_screen_->setId(-1);
    board_screen_->clearBoardData();
    profile_screen_->hide();
    stacked_widget_->setCurrentWidget(login_screen_);
    setWindowTitle("Chronos - Вход");
    if (isMaximized()) {
        showNormal();
    }
    resize(400, 600);
}

void MainWindow::switchToRegistration() {
    profile_screen_->hide();
    stacked_widget_->setCurrentWidget(registration_screen_);
    setWindowTitle("Chronos - Регистрация");
    if (isMaximized()) {
        showNormal();
    }
    resize(400, 600);
}

void MainWindow::switchToBoard(int board_id) {
    if (board_id != -1) {
        current_board_id_ = board_id;
    }

    board_screen_->setId(current_board_id_);
    board_screen_->reloadBoardData();

    profile_screen_->hide();
    stacked_widget_->setCurrentWidget(board_screen_);
    setWindowTitle("Chronos - Доска");
    showMaximized();

    if (network_manager_->hasToken()) {
        sync_coordinator_->syncAll();
        sync_coordinator_->loadAll();
    }
}
