#ifndef BOARD_HPP
#define BOARD_HPP

#include <optional>
#include <string>
#include <ctime>
#include <stdexcept>

class Board {
  public:
    int id_;
    int user_id_;
    std::string title_;
    std::string description_;
    bool is_private_;
    std::time_t created_at_;
    std::time_t updated_at_;

    Board() = default;

    Board(int id, int user_id, std::string title, std::string description, bool is_private,
          std::time_t created_at, std::time_t updated_at)
        : id_(id)
        , user_id_(user_id)
        , title_(std::move(title))
        , description_(std::move(description))
        , is_private_(is_private)
        , created_at_(created_at)
        , updated_at_(updated_at) {
        if (id_ < 0) {
            throw std::invalid_argument("Board ID cannot be negative");
        }
        if (user_id_ <= 0) {
            throw std::invalid_argument("Owner ID must be positive");
        }
        if (title_.empty() || title_.length() > 100) {
            throw std::invalid_argument("Board title must be between 1 and 100 characters");
        }
        if (description_.length() > 1000) {
            throw std::invalid_argument("Board description cannot exceed 1000 characters");
        }
    }
};

#endif // BOARD_HPP
