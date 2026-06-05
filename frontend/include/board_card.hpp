
#ifndef BOARD_CARD_HPP
#define BOARD_CARD_HPP

#include "board_screen.h"
#include "dd_task_card.hpp"
#include "network_manager.h"

#include <QFrame>
#include <QLabel>
#include <QMap>
#include <QScrollArea>

class BoardCard : public QFrame {
    Q_OBJECT

  public:
    explicit BoardCard(int board_id, QWidget* parent = nullptr);
    void setNetworkManager(NetworkManager* manager);

  private:
    int board_id_;

    bool should_be_delete_{false};

    NetworkManager* network_manager_{nullptr};

    QLabel* title_{nullptr};
    QLabel* active_tasks_count_{nullptr};
    QLabel* next_deadline_{nullptr};

    QScrollArea* boards_scroll_area{nullptr};
    QMap<int, DdTaskCard*> dd_task_cards_;
    QMap<int, BoardCard*> board_cards_;
};

#endif // BOARD_CARD_HPP
