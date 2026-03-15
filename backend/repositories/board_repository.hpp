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

    void save(Board& board);
    std::optional<Board> find_by_id(int board_id);

  private:
    ConnectionPool& pool_;

    void insert(Board& board);
    void update(const Board& board);
};

#endif // BOARD_REPOSITORY_HPP
