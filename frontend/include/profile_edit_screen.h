#ifndef PROFILE_EDIT_SCREEN_H
#define PROFILE_EDIT_SCREEN_H

#include "../sync/sync_coordinator.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QWidget>

class ProfileEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileEditScreen(QWidget* parent = nullptr);

    void setDatabase(QSqlDatabase db);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void reloadFromLocal();

  signals:
    void profileRequested();

  private slots:
    void onProfileEditRequest();

  private:
    QSqlDatabase db_;
    SyncCoordinator* sync_coordinator_{nullptr};
    QString pending_password_;

    QLabel* logo_label_{nullptr};
    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};

    QPushButton* save_button_{nullptr};
    QPushButton* cancel_button_{nullptr};

    QLineEdit* name_input_{nullptr};
    QLineEdit* email_input_{nullptr};
    QLineEdit* status_input_{nullptr};
    QLineEdit* password_input_{nullptr};

    void setupLayout();
};

#endif // PROFILE_EDIT_SCREEN_H
