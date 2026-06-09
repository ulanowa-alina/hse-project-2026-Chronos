#ifndef DD_TASK_CARD_H
#define DD_TASK_CARD_H

#include "network_manager.h"
#include "task_card.h"

#include <QFrame>
#include <QSqlDatabase>
#include <QVBoxLayout>

class DdTaskCard : public QFrame {
    Q_OBJECT

  public:
    explicit DdTaskCard(int task_id, int board_id, int status_id, QSqlDatabase db,
                        QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);

    void setCardData(const QString& title, const QString& description,
                     const QDateTime& deadline = QDateTime(), bool is_completed = false,
                     const QString& priority_color = QString());

  signals:
    void openBoardRequested(int board_id);

  private slots:
    void onBoardButtonClicked();

  private:
    int task_id_;
    int board_id_;
    int status_id_;
    QSqlDatabase db_;

    NetworkManager* network_manager_{nullptr};

    QPushButton* board_button_{nullptr};
    TaskCard* inner_task_card_{nullptr};
    QVBoxLayout* main_layout_{nullptr};

    void setupLayout();
    void freezeTaskCard();
    void loadBoardTitle();
};

#endif // DD_TASK_CARD_H
