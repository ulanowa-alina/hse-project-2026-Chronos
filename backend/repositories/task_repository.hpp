#ifndef TASK_REPOSITORY_HPP
#define TASK_REPOSITORY_HPP

#include "../models/task.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

class TaskRepository {
  public:
    explicit TaskRepository(pqxx::connection& conn);

    void save(Task& task);
    std::optional<Task> find_by_id(int task_id);

  private:
    pqxx::connection& conn_;
    std::string time_to_string(std::time_t t);

    void insert(Task& task);
    void update(const Task& task);
};
#endif // TASK_REPOSITORY_HPP
