#include "main_window.h"

#include <QStackedWidget>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    network_manager_ = new NetworkManager(this);

    stacked_widget_ = new QStackedWidget(this);
    setCentralWidget(stacked_widget_);

    login_screen_ = new LoginScreen(this);
    profile_screen_ = new ProfileScreen();
    registration_screen_ = new RegistrationScreen(this);
    board_screen_ = new BoardScreen(1, this);

    login_screen_->setNetworkManager(network_manager_);
    profile_screen_->setNetworkManager(network_manager_);
    registration_screen_->setNetworkManager(network_manager_);
    board_screen_->setNetworkManager(network_manager_);

    stacked_widget_->addWidget(login_screen_);
    stacked_widget_->addWidget(registration_screen_);
    stacked_widget_->addWidget(board_screen_);

    connect(login_screen_, &LoginScreen::loginRequested, this, &MainWindow::switchToBoard);
    connect(profile_screen_, &ProfileScreen::logoutRequested, this, &MainWindow::switchToLogin);
    connect(profile_screen_, &ProfileScreen::boardRequested, this, &MainWindow::switchToBoard);
    connect(registration_screen_, &RegistrationScreen::loginRequested, this,
            &MainWindow::switchToLogin);
    connect(registration_screen_, &RegistrationScreen::registrationRequested, this,
            &MainWindow::switchToBoard);
    connect(login_screen_, &LoginScreen::registrationRequested, this,
            &MainWindow::switchToRegistration);
    connect(board_screen_, &BoardScreen::openProfileScreen, this, &MainWindow::switchToProfile);

    setWindowTitle("Chronos");
    resize(400, 600);
    profile_screen_->setWindowTitle("Chronos - Профиль");
    profile_screen_->setFixedSize(420, 620);
}

void MainWindow::switchToProfile() {
    profile_screen_->show();
    profile_screen_->raise();
    profile_screen_->activateWindow();
}

void MainWindow::switchToLogin() {
    network_manager_->clearToken();
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

void MainWindow::switchToBoard() {
    profile_screen_->hide();
    stacked_widget_->setCurrentWidget(board_screen_);
    setWindowTitle("Chronos - Доска");
    showMaximized();
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
