#ifndef TASK_H
#define TASK_H

#include <ctime>
#include <optional>
#include <string>

class Task {
  public:
    int id_;
    int board_id_;
    std::string title_;
    std::string description_;
    std::time_t deadline_;
    std::string status_;
    int priority_;

    std::time_t created_at_;
    std::time_t updated_at_;

    Task() = default;
    Task(int id, int board_id, const std::string& title, const std::string& description,
         std::time_t deadline, const std::string& status, int priority, std::time_t created_at,
         std::time_t updated_at)
        : id_(id)
        , board_id_(board_id)
        , title_(title)
        , description_(description)
        , deadline_(deadline)
        , status_(status)
        , priority_(priority)
        , created_at_(created_at)
        , updated_at_(updated_at) {
    }
};

#endif // TASK_H
