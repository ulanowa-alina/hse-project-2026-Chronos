#include "task_repository.hpp"

#include <optional>
#include <stdexcept>

TaskRepository::TaskRepository(pqxx::connection& conn)
    : conn_(conn) {
}

std::string TaskRepository::time_to_string(std::time_t t) {
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buffer);
}

void TaskRepository::insert(Task& task) {
    pqxx::work txn(conn_);
    pqxx::result r = txn.exec_params("INSERT INTO tasks (board_id, title, description, deadline, "
                                     "status, priority, created_at, updated_at) "
                                     "VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW()) "
                                     "RETURNING id, "
                                     "EXTRACT(EPOCH FROM created_at)::bigint, "
                                     "EXTRACT(EPOCH FROM updated_at)::bigint",
                                     task.board_id_, task.title_, task.description_,
                                     task.deadline_ ? time_to_string(task.deadline_) : nullptr,
                                     task.status_, task.priority_);

    task.id_ = r[0][0].as<int>();
    task.created_at_ = static_cast<std::time_t>(r[0][1].as<long>());
    task.updated_at_ = static_cast<std::time_t>(r[0][2].as<long>());

    txn.commit();
}

void TaskRepository::update(const Task& task) {
    pqxx::work txn(conn_);
    txn.exec_params("UPDATE tasks SET board_id = $1, title = $2, description = $3, deadline = $4, "
                    "status = $5, priority = $6, updated_at = NOW() WHERE id = $7",
                    task.board_id_, task.title_, task.description_,
                    task.deadline_ ? time_to_string(task.deadline_) : nullptr, task.status_,
                    task.priority_, task.id_);
    txn.commit();
}

void TaskRepository::save(Task& task) {
    try {
        if (task.id_ < 0) {
            throw std::domain_error("Task ID cannot be negative (id: " + std::to_string(task.id_) +
                                    ")");
        } else if (task.id_ == 0) {
            insert(task);
        } else {
            update(task);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Task: ") + e.what());
    }
}

std::optional<Task> TaskRepository::find_by_id(int task_id) {
    try {
        pqxx::work txn(conn_);
        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, title, description, EXTRACT(EPOCH FROM deadline)::bigint AS "
            "deadline_sec, "
            "status, priority, EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
            "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
            "FROM tasks WHERE id = $1",
            task_id);

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        Task t;
        t.id_ = row["id"].as<int>();
        t.board_id_ = row["board_id"].as<int>();
        t.title_ = row["title"].as<std::string>();
        t.description_ = row["description"].as<std::string>();
        t.status_ = row["status"].as<std::string>();
        t.priority_ = row["priority"].as<int>();
        t.deadline_ = row["deadline_sec"].is_null() ? 0 : row["deadline_sec"].as<long>();
        t.created_at_ = row["created_sec"].as<long>();
        t.updated_at_ = row["updated_sec"].as<long>();
        txn.commit();
        return t;

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Task: ") + e.what());
    }
}
