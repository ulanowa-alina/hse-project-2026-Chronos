#ifndef PROFILE_EDIT_SCREEN_H
#define PROFILE_EDIT_SCREEN_H

#include "../sync/sync_coordinator.hpp"

#include <QByteArray>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QSqlDatabase>
#include <QWidget>

class ProfileEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit ProfileEditScreen(QWidget* parent = nullptr);

    void setDatabase(QSqlDatabase db);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void setNetworkManager(NetworkManager* manager);
    void reloadFromLocal();

  signals:
    void profileRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onProfileEditRequest();
    void onAvatarPickRequested();
    void onAvatarDeleteRequested();
    void onAvatarImageDownloaded(QNetworkReply* reply);

  private:
    NetworkManager* network_manager_{nullptr};
    QNetworkAccessManager* avatar_network_manager_{nullptr};

    QSqlDatabase db_;
    SyncCoordinator* sync_coordinator_{nullptr};
    QString pending_password_;

    QString current_avatar_s3_key_;
    QString selected_avatar_file_path_;
    QString original_name_;
    QString original_email_;
    QString original_status_;
    bool avatar_delete_requested_ = false;

    QLabel* logo_label_{nullptr};
    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};

    QPushButton* save_button_{nullptr};
    QPushButton* cancel_button_{nullptr};
    QPushButton* avatar_button_{nullptr};
    QPushButton* delete_avatar_button_{nullptr};

    QLineEdit* name_input_{nullptr};
    QLineEdit* email_input_{nullptr};
    QLineEdit* status_input_{nullptr};
    QLineEdit* password_input_{nullptr};

    void setupLayout();
    void showEvent(QShowEvent* event) override;
    void updateAvatarButtonPreview(const QString& file_path);
    auto hasPendingTextChanges() const -> bool;
    void sendProfileUpdate();
    void sendAvatarUpload();
    void sendAvatarDelete();
    auto parseErrorMessage(const QByteArray& data) const -> QString;
    void resetAvatarSelection();
    void updateAvatarDeleteButtonState();
    void loadRemoteAvatar(const QString& avatar_s3_key);
};

#endif // PROFILE_EDIT_SCREEN_H
