#ifndef BOARD_HPP
#define BOARD_HPP

#include <pqxx/pqxx>
#include <string>
#include <optional>

class Board {
  public:
    int id_;
    int user_id_;
    std::string title_;

    Board() = default;
    Board(int id, int user_id, const std::string& title);

    void save(pqxx::connection& conn);
    static std::optional<Board> find_by_id(pqxx::connection& conn, int id);
};

#endif // BOARD_HPP
