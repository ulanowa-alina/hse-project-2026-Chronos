#include "main_window.h"

#include <QSqlQuery>
#include <QStackedWidget>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    network_manager_ = new NetworkManager(this);

    local_db_ = new LocalDatabaseManager(this);
    local_db_->open("chronos_local.db");
    local_db_->createDb("../sql/local_init.sql");


    QSqlDatabase db = local_db_->getDatabase();
    sync_manager_ = new SyncManager(db, network_manager_);

    stacked_widget_ = new QStackedWidget(this);
    setCentralWidget(stacked_widget_);

    login_screen_ = new LoginScreen(this);
    profile_screen_ = new ProfileScreen(this);
    profile_edit_screen_ = new ProfileEditScreen(this);
    registration_screen_ = new RegistrationScreen(this);
    board_screen_ = new BoardScreen(current_board_id_, this);

    login_screen_->setNetworkManager(network_manager_);
    profile_screen_->setNetworkManager(network_manager_);
    profile_edit_screen_->setNetworkManager(network_manager_);
    registration_screen_->setNetworkManager(network_manager_);
    board_screen_->setNetworkManager(network_manager_);

    login_screen_->setSyncManager(sync_manager_);

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

    setWindowTitle("Chronos");
    resize(400, 600);
}

void MainWindow::switchToProfile() {
    stacked_widget_->setCurrentWidget(profile_screen_);
    setWindowTitle("Chronos - Профиль");

    if (isMaximized()) {
        showNormal();
    }
    resize(420, 620);
}

void MainWindow::switchToProfileEdit() {
    stacked_widget_->setCurrentWidget(profile_edit_screen_);
    setWindowTitle("Chronos - Редактирование");

    if (isMaximized()) {
        showNormal();
    }
    resize(420, 620);
}
void MainWindow::switchToLogin() {
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
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
