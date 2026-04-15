#include "task_repository.hpp"

#include <optional>
#include <stdexcept>

TaskRepository::TaskRepository(ConnectionPool& pool)
    : pool_(pool) {
}

std::string TaskRepository::time_to_string(std::time_t t) {
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buffer);
}

Task TaskRepository::insert(const Task& task) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    const std::optional<std::string> deadline =
        task.deadline_ ? std::optional<std::string>{time_to_string(*task.deadline_)} : std::nullopt;

    pqxx::result r = txn.exec_params("INSERT INTO tasks (board_id, title, description, deadline, "
                                     "status_id, priority_color, created_at, updated_at) "
                                     "VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW()) "
                                     "RETURNING id, "
                                     "EXTRACT(EPOCH FROM created_at)::bigint, "
                                     "EXTRACT(EPOCH FROM updated_at)::bigint",
                                     task.board_id_, task.title_, task.description_, deadline,
                                     task.status_id_, task.priority_color_);

    txn.commit();

    return Task(r[0][0].as<int>(), task.board_id_, task.title_, task.description_, task.deadline_,
                task.status_id_, task.priority_color_, static_cast<std::time_t>(r[0][1].as<long>()),
                static_cast<std::time_t>(r[0][2].as<long>()));
}

Task TaskRepository::update(const Task& task) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    const std::optional<std::string> deadline =
        task.deadline_ ? std::optional<std::string>{time_to_string(*task.deadline_)} : std::nullopt;

    pqxx::result r = txn.exec_params(
        "UPDATE tasks SET board_id = $1, title = $2, description = $3, deadline = $4, "
        "status_id = $5, priority_color = $6, updated_at = NOW() WHERE id = $7 "
        "RETURNING EXTRACT(EPOCH FROM updated_at)::bigint",
        task.board_id_, task.title_, task.description_, deadline, task.status_id_,
        task.priority_color_, task.id_);

    txn.commit();
    Task updated_task = task;
    if (!r.empty()) {
        updated_task.updated_at_ = static_cast<std::time_t>(r[0][0].as<long>());
    }
    return updated_task;
}

Task TaskRepository::save(const Task& task) {
    try {
        if (task.id_ == 0) {
            return insert(task);
        } else {
            return update(task);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Task: ") + e.what());
    }
}

std::optional<Task> TaskRepository::find_by_id(int task_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, title, description, EXTRACT(EPOCH FROM deadline)::bigint AS "
            "deadline_sec, "
            "status_id, priority_color, EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
            "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
            "FROM tasks WHERE id = $1",
            task_id);

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];

        txn.commit();

        return Task(row["id"].as<int>(), row["board_id"].as<int>(), row["title"].as<std::string>(),
                    row["description"].as<std::string>(),
                    row["deadline_sec"].is_null()
                        ? std::optional<std::time_t>{}
                        : std::optional<std::time_t>{row["deadline_sec"].as<long>()},
                    row["status_id"].as<int>(), row["priority_color"].as<std::string>(),
                    static_cast<std::time_t>(row["created_sec"].as<long>()),
                    static_cast<std::time_t>(row["updated_sec"].as<long>()));

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Task: ") + e.what());
    }
}

std::vector<Task> TaskRepository::find_by_board_id(int board_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, title, description, EXTRACT(EPOCH FROM deadline)::bigint AS "
            "deadline_sec, "
            "status_id, priority_color, EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
            "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
            "FROM tasks WHERE board_id = $1 ORDER BY created_at ASC, id ASC",
            board_id);

        std::vector<Task> tasks;
        tasks.reserve(r.size());

        for (const auto& row : r) {
            tasks.emplace_back(
                row["id"].as<int>(), row["board_id"].as<int>(), row["title"].as<std::string>(),
                row["description"].is_null() ? "" : row["description"].as<std::string>(),
                row["deadline_sec"].is_null() ? std::optional<std::time_t>{}
                                              : std::optional<std::time_t>{static_cast<std::time_t>(
                                                    row["deadline_sec"].as<long>())},
                row["status_id"].as<int>(), row["priority_color"].as<std::string>(),
                static_cast<std::time_t>(row["created_sec"].as<long>()),
                static_cast<std::time_t>(row["updated_sec"].as<long>()));
        }

        txn.commit();
        return tasks;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Tasks by board id: ") + e.what());
    }
}

bool TaskRepository::delete_by_id(int task_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        const pqxx::result r =
            txn.exec_params("DELETE FROM tasks WHERE id = $1 RETURNING id", task_id);

        txn.commit();
        return !r.empty();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to delete Task: ") + e.what());
    }
}
