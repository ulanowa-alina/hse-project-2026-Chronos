#ifndef TASK_H
#define TASK_H

#include <ctime>
#include <optional>
#include <string>
#include <stdexcept>

class Task {
  public:
    int id_;
    int board_id_;
    std::string title_;
    std::string description_;
    std::time_t deadline_;
    int status_id_;
    int priority_;

    std::time_t created_at_;
    std::time_t updated_at_;

    Task() = default;
    Task(int id, int board_id, std::string title, std::string description, std::time_t deadline,
         int status_id, int priority, std::time_t created_at, std::time_t updated_at)
        : id_(id)
        , board_id_(board_id)
        , title_(std::move(title))
        , description_(std::move(description))
        , deadline_(deadline)
        , status_id_(status_id)
        , priority_(priority)
        , created_at_(created_at)
        , updated_at_(updated_at) {

        if (id_ < 0) {
            throw std::invalid_argument("Task ID cannot be negative");
        }
        if (board_id_ <= 0) {
            throw std::invalid_argument("Board ID must be positive");
        }
        if (status_id_ <= 0) {
            throw std::invalid_argument("Status ID must be positive");
        }
        if (title.empty() || title.length() > 100) {
            throw std::invalid_argument("Title must be between 1 and 100 characters");
        }
        if (description.length() > 1000) {
            throw std::invalid_argument("Description is too long (max 1000)");
        }
    }
};

#endif // TASK_H
