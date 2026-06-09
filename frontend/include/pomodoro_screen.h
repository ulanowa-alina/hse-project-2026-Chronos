#ifndef POMODORO_SCREEN_H
#define POMODORO_SCREEN_H

#include "circular_progress.h"
#include "network_manager.h"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class PomodoroScreen : public QWidget {
    Q_OBJECT

  public:
    explicit PomodoroScreen(QWidget* parent = nullptr);

    void setNetworkManager(NetworkManager* manager);
    void setDatabase(QSqlDatabase db);

  signals:
    void openProfileScreen();
    void sessionSaved();

  private slots:
    void onNetworkResponse(const QString& endpoint, const QByteArray& data, int code);
    void onStartFocusClicked();
    void onGoalConfirmed();
    void onSkipGoalClicked();
    void onStopClicked();
    void onTimerTick();

  private:
    enum class ScreenState { Welcome, GoalSelection, Timer };

    NetworkManager* network_manager_{nullptr};
    QSqlDatabase db_;

    QStackedWidget* stacked_widget_{nullptr};

    QWidget* welcome_widget_{nullptr};
    QLabel* welcome_label_{nullptr};
    QPushButton* start_focus_button_{nullptr};

    QWidget* goal_widget_{nullptr};
    QLabel* goal_label_{nullptr};
    QSpinBox* goal_spinbox_{nullptr};
    QPushButton* confirm_goal_button_{nullptr};
    QPushButton* skip_goal_button_{nullptr};

    QWidget* timer_widget_{nullptr};
    QLabel* state_label_{nullptr};
    CircularProgress* circular_progress_{nullptr};
    QPushButton* stop_button_{nullptr};

    QTimer* timer_{nullptr};
    ScreenState current_state_{ScreenState::Welcome};
    bool is_work_phase_{true};
    int remaining_seconds_{0};
    int total_work_seconds_{30 * 60};
    int default_work_seconds_{30 * 60};
    int total_break_seconds_{5 * 60};
    int completed_cycles_{0};
    int goal_minutes_{0};
    bool has_goal_{false};
    int total_work_time_seconds_{0};
    int last_saved_work_minutes_{0};
    int local_session_id_{0};

    void setupWelcomeScreen();
    void setupGoalSelectionScreen();
    void setupTimerScreen();
    void switchState(ScreenState state);
    void startTimer();
    void stopTimer();
    void saveSession();
    void saveLocalSession();
    void saveLocalSessionFromServer(const QJsonObject& obj);
    int nextWorkPhaseSeconds() const;
    QString formatTime(int seconds);
};

#endif // POMODORO_SCREEN_H
