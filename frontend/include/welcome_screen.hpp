#ifndef WELCOME_SCREEN_HPP
#define WELCOME_SCREEN_HPP

#include "blue_oval.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class WelcomeScreen : public QWidget {
    Q_OBJECT
  public:
    explicit WelcomeScreen(QWidget* parent = nullptr);

  signals:
    void loginRequested();

  private:
    QPushButton* start_button_{nullptr};

    void setupLayout();
};

#endif // WELCOME_SCREEN_HPP
