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
    profile_screen_ = new ProfileScreen(this);
    registration_screen_ = new RegistrationScreen(this);

    login_screen_->set_network_manager(network_manager_);
    profile_screen_->set_network_manager(network_manager_);
    registration_screen_->set_network_manager(network_manager_);

    stacked_widget_->addWidget(login_screen_);
    stacked_widget_->addWidget(profile_screen_);
    stacked_widget_->addWidget(registration_screen_);

    connect(login_screen_, &LoginScreen::loginRequested, this, &MainWindow::switchToProfile);
    connect(profile_screen_, &ProfileScreen::logoutRequested, this, &MainWindow::switchToLogin);
    connect(registration_screen_, &RegistrationScreen::loginRequested, this,
            &MainWindow::switchToLogin);
    connect(registration_screen_, &RegistrationScreen::registrationRequested, this,
            &MainWindow::switchToProfile);
    connect(login_screen_, &LoginScreen::registrationRequested, this,
            &MainWindow::switchToRegistration);

    setWindowTitle("Chronos");
    resize(400, 600);
}

void MainWindow::switchToProfile() {
    stacked_widget_->setCurrentWidget(profile_screen_);
    setWindowTitle("Chronos - Профиль");
}

void MainWindow::switchToLogin() {
    stacked_widget_->setCurrentWidget(login_screen_);
    setWindowTitle("Chronos - Вход");
}

void MainWindow::switchToRegistration() {
    stacked_widget_->setCurrentWidget(registration_screen_);
    setWindowTitle("Chronos - Регистрация");
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
