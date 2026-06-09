#ifndef PROFILE_SCREEN_H
#define PROFILE_SCREEN_H

#include "../sync/sync_coordinator.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSqlDatabase>
#include <QVBoxLayout>
#include <QWidget>

class ProfileScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileScreen(QWidget* parent = nullptr);

    void setDatabase(QSqlDatabase db);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void reloadFromLocal();

  signals:
    void logoutRequested();
    void boardRequested();
    void profileEditRequested();
    void openDashboardScreen();

  private:
    QSqlDatabase db_;
    SyncCoordinator* sync_coordinator_{nullptr};

    QPushButton* edit_button_{nullptr};
    QPushButton* logout_button_{nullptr};
    QPushButton* logo_button_{nullptr};

    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};
    QLabel* status_label_{nullptr};
    QLabel* email_label_{nullptr};

    void setupLayout();
};

#endif // PROFILE_SCREEN_H
