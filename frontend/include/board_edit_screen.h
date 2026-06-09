#ifndef BOARD_EDIT_SCREEN_H
#define BOARD_EDIT_SCREEN_H

#include "../sync/sync_coordinator.hpp"
#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class BoardEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit BoardEditScreen(int board_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setSyncCoordinator(SyncCoordinator* coordinator);
    void setDatabase(QSqlDatabase db);
    void setBoardId(int board_id);
    void loadBoardData();

  signals:
    void boardUpdated();
    void closeRequested();

  private slots:
    void onUpdateBoardRequest();
    void onCloseRequest();

  private:
    NetworkManager* network_manager_{nullptr};
    SyncCoordinator* sync_coordinator_{nullptr};
    QSqlDatabase db_;
    int board_id_{-1};

    QPushButton* update_button_{nullptr};
    QPushButton* cancel_button_{nullptr};
    QPushButton* close_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* title_label_{nullptr};
    QLabel* error_label_{nullptr};

    QLineEdit* title_input_{nullptr};
    QTextEdit* description_input_{nullptr};

    void setupLayout();
    void showErrorMessage(const QString& message);
    void clearErrorMessage();
};

#endif // BOARD_EDIT_SCREEN_H
