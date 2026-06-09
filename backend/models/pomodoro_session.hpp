#ifndef POMODORO_SESSION_H
#define POMODORO_SESSION_H

#include <ctime>
#include <optional>
#include <stdexcept>
#include <string>

class PomodoroSession {
  public:
    int id_;
    int user_id_;
    std::optional<int> goal_minutes_;
    int work_duration_seconds_;
    int break_duration_seconds_;
    int completed_cycles_;
    std::time_t started_at_;
    std::optional<std::time_t> completed_at_;

    PomodoroSession() = default;
    PomodoroSession(int id, int user_id, std::optional<int> goal_minutes, int work_duration_seconds,
                    int break_duration_seconds, int completed_cycles, std::time_t started_at,
                    std::optional<std::time_t> completed_at)
        : id_(id)
        , user_id_(user_id)
        , goal_minutes_(goal_minutes)
        , work_duration_seconds_(work_duration_seconds)
        , break_duration_seconds_(break_duration_seconds)
        , completed_cycles_(completed_cycles)
        , started_at_(started_at)
        , completed_at_(completed_at) {
        if (user_id_ <= 0) {
            throw std::invalid_argument("User ID must be positive");
        }
        if (goal_minutes_.has_value() && goal_minutes_.value() <= 0) {
            throw std::invalid_argument("Goal minutes must be positive if set");
        }
        if (work_duration_seconds_ <= 0) {
            throw std::invalid_argument("Work duration must be positive");
        }
        if (break_duration_seconds_ <= 0) {
            throw std::invalid_argument("Break duration must be positive");
        }
        if (completed_cycles_ < 0) {
            throw std::invalid_argument("Completed cycles cannot be negative");
        }
        if (started_at_ < 0) {
            throw std::invalid_argument("Started at cannot be negative");
        }
        if (completed_at_.has_value() && completed_at_.value() < 0) {
            throw std::invalid_argument("Completed at cannot be negative");
        }
    }
};

#endif // POMODORO_SESSION_H
