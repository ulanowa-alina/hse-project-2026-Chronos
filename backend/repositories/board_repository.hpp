#ifndef BOARD_REPOSITORY_HPP
#define BOARD_REPOSITORY_HPP
#include "../models/board.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

class BoardRepository {
  public:
    explicit BoardRepository(pqxx::connection& conn);

    void save(Board& board);
    std::optional<Board> find_by_id(int board_id);

  private:
    pqxx::connection& conn_;

    void insert(Board& board);
    void update(const Board& board);
};

#endif // BOARD_REPOSITORY_HPP
