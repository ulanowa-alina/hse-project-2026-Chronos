#include "main_window.h"

#include <QStackedWidget>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    stacked_widget_ = new QStackedWidget(this);
    setCentralWidget(stacked_widget_);

    login_screen_ = new LoginScreen(this);
    profile_screen_ = new ProfileScreen(this);

    stacked_widget_->addWidget(login_screen_);
    stacked_widget_->addWidget(profile_screen_);

    connect(login_screen_, &LoginScreen::loginRequested, this, &MainWindow::switchToProfile);
    connect(profile_screen_, &ProfileScreen::logoutRequested, this, &MainWindow::switchToLogin);

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
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
