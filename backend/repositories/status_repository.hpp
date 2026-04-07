#ifndef STATUS_REPOSITORY_HPP
#define STATUS_REPOSITORY_HPP

#include "../models/status.hpp"
#include "../src/db/connection_pool.hpp"

#include <optional>
#include <string>
#include <vector>

class StatusRepository {
  public:
    explicit StatusRepository(ConnectionPool& pool);

    Status create(int board_id, const std::string& name, int position);

    void create_defaults_for_board(int board_id);

    std::optional<Status> find_by_board_and_name(int board_id, const std::string& name);
    std::optional<Status> find_by_id(int status_id);
    std::vector<Status> find_by_board_id(int board_id);

  private:
    ConnectionPool& pool_;
};

#endif // STATUS_REPOSITORY_HPP
