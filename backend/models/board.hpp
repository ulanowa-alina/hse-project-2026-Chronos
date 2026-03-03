#ifndef BOARD_HPP
#define BOARD_HPP

#include <optional>
#include <string>

class Board {
  public:
    int id_;
    int user_id_;
    std::string title_;

    Board() = default;
    Board(int id, int user_id, const std::string& title)
        : id_(id)
        , user_id_(user_id)
        , title_(title) {
    }
};

#endif // BOARD_HPP
