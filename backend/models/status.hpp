#ifndef STATUS_HPP
#define STATUS_HPP

#include <stdexcept>
#include <string>

class Status {
  public:
    int id_;
    int board_id_;
    std::string name_;
    int position_;

    Status() = default;

    Status(int id, int board_id, std::string name, int position)
        : id_(id)
        , board_id_(board_id)
        , name_(std::move(name))
        , position_(position) {
        if (id_ < 0) {
            throw std::invalid_argument("Status ID cannot be negative");
        }
        if (board_id_ <= 0) {
            throw std::invalid_argument("Board ID must be positive");
        }
        if (name_.empty() || name_.length() > 50) {
            throw std::invalid_argument("Status name must be between 1 and 50 characters");
        }
        if (position_ < 0) {
            throw std::invalid_argument("Status position cannot be negative");
        }
    }
};

#endif // STATUS_HPP
