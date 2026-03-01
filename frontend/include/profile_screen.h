#ifndef PROFILE_SCREEN_H
#define PROFILE_SCREEN_H

#include "network_manager.h"

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

    void set_network_manager(NetworkManager* manager);

  signals:
    void logoutRequested();

  private slots:
    void on_network_response(const QString& endpoint, const QByteArray& data, int code);

  private:
    NetworkManager* network_manager_{nullptr};

    QPushButton* edit_button_{nullptr};
    QPushButton* logout_button_{nullptr};

    QLabel* avatar_label_{nullptr};
    QLabel* name_label_{nullptr};
    QLabel* status_label_{nullptr};
    QLabel* email_label_{nullptr};
    QLabel* logo_label_{nullptr};

    void setupLayout();
    void showEvent(QShowEvent* event) override;
};

#endif // PROFILE_SCREEN_H
