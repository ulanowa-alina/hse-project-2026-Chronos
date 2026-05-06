#ifndef BOARD_REPOSITORY_HPP
#define BOARD_REPOSITORY_HPP

#include "../models/board.hpp"
#include "../src/db/connection_pool.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

class BoardRepository {
  public:
    explicit BoardRepository(ConnectionPool& pool);

    Board save(const Board& board);
    std::optional<Board> find_by_id(int board_id);
    std::optional<Board> find_by_user_id(int user_id);
    std::vector<Board> find_all_by_user_id(int user_id);
    bool delete_by_id(int board_id);

  private:
    ConnectionPool& pool_;

    Board insert(const Board& board);
    Board update(const Board& board);
};

#endif // BOARD_REPOSITORY_HPP
