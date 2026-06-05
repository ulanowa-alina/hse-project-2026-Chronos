#ifndef DASHBOARD_SCREEN_HPP
#define DASHBOARD_SCREEN_HPP

#include "network_manager.h"

#include <QLabel>
#include <QPushButton>
#include <QWidget>
class DashboardScreen : public QWidget {
    Q_OBJECT

  public:
    explicit DashboardScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);

  signals:
    void openProfileScreen();
    void openBoardScreen(int board_id);
    void openPomodoroScreen();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray, int code);
    void onBoardCreateRequest();
    void onProfileRequest();
    void onPomodoroRequest();
    void onBoardRequest(int board_id);

  private:
    NetworkManager* network_manager_{nullptr};

    QPushButton* profile_button_{nullptr};
    QPushButton* add_board_button_{nullptr};
    QPushButton* pomodoro_button_{nullptr};
    QPushButton* log_out_button_{nullptr};
    QPushButton* big_add_board_button_{nullptr};

    QLabel* logo_label_{nullptr};
    QLabel* dashboard_label_{nullptr};
    QLabel* active_tasks_counter_{nullptr};
    QLabel* focus_hours_counter_{nullptr};
    QLabel* completed_tasks_counter_{nullptr};
    QLabel* next_dd_label_{nullptr};
    QLabel* boards_label_{nullptr};

    void setupLayout();
};
#endif // DASHBOARD_SCREEN_HPP
