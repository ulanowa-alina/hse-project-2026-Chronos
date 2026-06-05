
#ifndef DD_TASK_CARD_HPP
#define DD_TASK_CARD_HPP

#include "network_manager.h"
#include "task_card.h"

#include <QFrame>

class DdTaskCard : public TaskCard {
    Q_OBJECT

  public:
    explicit DdTaskCard(int task_id, int board_id, int status_id, QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);

  private:
};

#endif // DD_TASK_CARD_HPP
