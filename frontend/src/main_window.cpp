#include "main_window.h"

#include <QStackedWidget>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    stackedWidget_ = new QStackedWidget(this);
    profileScreen_ = new ProfileScreen(this);
    stackedWidget_->addWidget(profileScreen_);
    setCentralWidget(stackedWidget_);
    setWindowTitle("Chronos");
    resize(400, 600);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
