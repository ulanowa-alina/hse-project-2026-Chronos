#ifndef BOARD_CREATE_SCREEN_H
#define BOARD_CREATE_SCREEN_H

#include "../local_repositories/local_board_repository.hpp"
#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class BoardCreateScreen : public QWidget {
    Q_OBJECT

  public:
    explicit BoardCreateScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void setDatabase(QSqlDatabase* db);
    void setUserId(int user_id);

  signals:
    void boardCreated();
    void closeRequested();

  private slots:
    void onCreateBoardRequest();
    void onCloseRequest();

  private:
    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};
    QSqlDatabase* db_{nullptr};
    int user_id_{-1};

    QPushButton* create_button_{nullptr};
    QPushButton* cancel_button_{nullptr};
    QPushButton* close_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* title_label_{nullptr};
    QLabel* error_label_{nullptr};

    QLineEdit* title_input_{nullptr};
    QTextEdit* description_input_{nullptr};

    void setupLayout();
    void clearFields();
    void showErrorMessage(const QString& message);
    void clearErrorMessage();
};

#endif // BOARD_CREATE_SCREEN_H
