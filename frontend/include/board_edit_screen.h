#ifndef BOARD_EDIT_SCREEN_H
#define BOARD_EDIT_SCREEN_H

#include "network_manager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class BoardEditScreen : public QWidget {
    Q_OBJECT

  public:
    explicit BoardEditScreen(int board_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setBoardId(int board_id);
    void loadBoardData();

  signals:
    void boardUpdated();
    void closeRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onUpdateBoardRequest();
    void onCloseRequest();

  private:
    NetworkManager* network_manager_{nullptr};
    int board_id_{-1};

    QPushButton* update_button_{nullptr};
    QPushButton* cancel_button_{nullptr};
    QPushButton* close_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* title_label_{nullptr};

    QLineEdit* title_input_{nullptr};
    QTextEdit* description_input_{nullptr};

    void setupLayout();
};

#endif // BOARD_EDIT_SCREEN_H
