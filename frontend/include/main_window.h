#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "login_screen.h"
#include "profile_screen.h"
#include <QMainWindow>
#include <QStackedWidget>
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow(QWidget* parent = nullptr);

  private slots:
    void switchToProfile();
    void switchToLogin();
    //void switchToRegistration(); потом сделаю
  private:
    QStackedWidget* stacked_widget_{
        nullptr}; // <-- через эту штуку будем переключаться между экранами
    LoginScreen* login_screen_{nullptr};
    ProfileScreen* profile_screen_{nullptr};

};


#endif // MAIN_WINDOW_H
