#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "profile_screen.h"

#include <QMainWindow>
#include <QStackedWidget>
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow(QWidget* parent = nullptr);

  private:
    QStackedWidget* stackedWidget_{
        nullptr}; // <-- через эту штуку будем переключаться между экранами
    ProfileScreen* profileScreen_{nullptr};
};

#endif // MAIN_WINDOW_H
