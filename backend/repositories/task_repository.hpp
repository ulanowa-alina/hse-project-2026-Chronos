#ifndef TASK_REPOSITORY_HPP
#define TASK_REPOSITORY_HPP

#include "../models/task.hpp"
#include "../src/db/connection_pool.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

class TaskRepository {
  public:
    explicit TaskRepository(ConnectionPool& pool);

    Task save(const Task& task);
    std::optional<Task> find_by_id(int task_id);
    std::vector<Task> find_by_board_id(int board_id);

  private:
    ConnectionPool& pool_;
    std::string time_to_string(std::time_t t);

    Task insert(const Task& task);
    Task update(const Task& task);
};
#endif // TASK_REPOSITORY_HPP
