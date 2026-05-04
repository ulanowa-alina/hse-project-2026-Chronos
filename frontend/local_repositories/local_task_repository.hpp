#ifndef LOCAL_TASK_REPOSITORY_HPP
#define LOCAL_TASK_REPOSITORY_HPP

#include "../local_models/local_task.hpp"

#include <QSqlDatabase>
#include <optional>
#include <vector>

class LocalTaskRepository {
  public:
    explicit LocalTaskRepository(QSqlDatabase& db);

    LocalTask save(const LocalTask& task);
    std::optional<LocalTask> findById(int task_id);
    std::vector<LocalTask> findByBoardId(int board_id);
    std::vector<LocalTask> findByStatusId(int status_id);
    std::vector<LocalTask> findUnsynced();
    void deleteById(int task_id);
    void markSynced(int task_id);

  private:
    QSqlDatabase& db_;

    LocalTask insert(const LocalTask& task);
    LocalTask update(const LocalTask& task);
};

#endif // LOCAL_TASK_REPOSITORY_HPP
