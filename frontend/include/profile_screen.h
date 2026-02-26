#ifndef PROFILE_SCREEN_H
#define PROFILE_SCREEN_H

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class ProfileScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileScreen(QWidget* parent = nullptr);

  signals:
    void logoutRequested();

  private:
    QPushButton* edit_button_{nullptr};
    QPushButton* logout_button_{nullptr};

    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};
    QLabel* status_label_{nullptr};
    QLabel* email_label_{nullptr};
    QLabel* logo_label_{nullptr};

    void setupLayout();
};

#endif // PROFILE_SCREEN_H
