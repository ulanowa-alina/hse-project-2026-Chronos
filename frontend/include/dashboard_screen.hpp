#ifndef DASHBOARD_SCREEN_HPP
#define DASHBOARD_SCREEN_HPP

#include "board_card.hpp"
#include "dd_task_card.hpp"
#include "network_manager.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class DashboardScreen : public QWidget {
    Q_OBJECT

  public:
    explicit DashboardScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void reloadDashboardData();

  signals:
    void openProfileScreen();
    void openBoardScreen(int board_id);
    void openBoardCreateScreen();
    void openPomodoroScreen();
    void logoutRequested();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray data, int code);
    void onBoardCreateRequest();
    void onProfileRequest();
    void onPomodoroRequest();
    void onBoardRequest(int board_id);
    void onLogoutRequest();

  private:
    NetworkManager* network_manager_{nullptr};

    QWidget* sidebar_widget_{nullptr};
    QLabel* logo_label_{nullptr};
    QPushButton* dashboard_button_{nullptr};
    QPushButton* add_board_button_{nullptr};
    QPushButton* profile_button_{nullptr};
    QPushButton* pomodoro_button_{nullptr};
    QPushButton* logout_button_{nullptr};

    QWidget* header_widget_{nullptr};
    QLabel* dashboard_title_label_{nullptr};

    QFrame* stats_frame_{nullptr};
    QFrame* active_tasks_frame_{nullptr};
    QLabel* active_tasks_label_{nullptr};
    QLabel* active_tasks_value_{nullptr};
    QFrame* focus_hours_frame_{nullptr};
    QLabel* focus_hours_label_{nullptr};
    QLabel* focus_hours_value_{nullptr};
    QFrame* completed_tasks_frame_{nullptr};
    QLabel* completed_tasks_label_{nullptr};
    QLabel* completed_tasks_value_{nullptr};

    QFrame* deadlines_frame_{nullptr};
    QLabel* deadlines_title_label_{nullptr};
    QWidget* deadlines_container_{nullptr};
    QLabel* no_deadlines_label_{nullptr};
    QVector<DdTaskCard*> deadline_cards_;

    QFrame* boards_frame_{nullptr};
    QLabel* boards_title_label_{nullptr};
    QScrollArea* boards_scroll_area_{nullptr};
    QWidget* boards_container_{nullptr};
    QPushButton* add_board_card_button_{nullptr};
    QVector<BoardCard*> board_cards_;
    QMap<int, QJsonObject> boards_data_;

    int active_tasks_count_{0};
    int focus_hours_count_{0};
    int completed_tasks_count_{0};

    void setupLayout();
    void setupSidebar();
    void setupHeader();
    void setupStatsFrame();
    void setupDeadlinesFrame();
    void setupBoardsFrame();
    void loadStatistics();
    void loadDeadlines();
    void loadBoards();
    void updateDashboardButton();
};
#endif // DASHBOARD_SCREEN_HPP
