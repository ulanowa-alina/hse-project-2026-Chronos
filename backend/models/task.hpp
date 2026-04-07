#ifndef TASK_H
#define TASK_H

#include <ctime>
#include <optional>
#include <stdexcept>
#include <string>

class Task {
  public:
    int id_;
    int board_id_;
    std::string title_;
    std::string description_;
    std::optional<std::time_t> deadline_;
    int status_id_;
    std::string priority_color_;

    std::time_t created_at_;
    std::time_t updated_at_;

    Task() = default;
    Task(int id, int board_id, std::string title, std::string description,
         std::optional<std::time_t> deadline,
         int status_id, std::string priority_color, std::time_t created_at, std::time_t updated_at)
        : id_(id)
        , board_id_(board_id)
        , title_(std::move(title))
        , description_(std::move(description))
        , deadline_(deadline)
        , status_id_(status_id)
        , priority_color_(std::move(priority_color))
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
        if (title_.empty() || title_.length() > 100) {
            throw std::invalid_argument("Title must be between 1 and 100 characters");
        }
        if (description_.length() > 1000) {
            throw std::invalid_argument("Description is too long (max 1000)");
        }
        if (priority_color_.empty() || priority_color_.length() > 50) {
            throw std::invalid_argument("Priority color must be between 1 and 50 characters");
        }
        if (created_at_ < 0 || updated_at_ < 0) {
            throw std::invalid_argument("Task timestamps cannot be negative");
        }
        if (deadline_.has_value() && *deadline_ < 0) {
            throw std::invalid_argument("Task deadline cannot be negative");
        }
    }
};

#endif // TASK_H
