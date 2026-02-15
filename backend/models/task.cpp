#include "task.hpp"

#include <ctime>
#include <pqxx/pqxx>
#include <sstream>
#include <stdexcept>

static std::string time_to_string(std::time_t t) {
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buffer);
}

void Task::save(pqxx::connection& conn) {
    try {
        pqxx::work txn(conn);

        if (id == 0) {
            pqxx::result r =
                txn.exec_params("INSERT INTO tasks (board_id, title, description, deadline, "
                                "status, priority, created_at, updated_at) "
                                "VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW()) "
                                "RETURNING id, "
                                "EXTRACT(EPOCH FROM created_at)::bigint, "
                                "EXTRACT(EPOCH FROM updated_at)::bigint",
                                board_id, title, description,
                                deadline ? time_to_string(deadline) : nullptr, status, priority);

            id = r[0][0].as<int>();
            created_at = static_cast<std::time_t>(r[0][1].as<long>());
            updated_at = static_cast<std::time_t>(r[0][2].as<long>());

        } else {
            txn.exec_params(
                "UPDATE tasks SET board_id = $1, title = $2, description = $3, deadline = $4, "
                "status = $5, priority = $6, updated_at = NOW() WHERE id = $7",
                board_id, title, description, deadline ? time_to_string(deadline) : nullptr, status,
                priority, id);
        }

        txn.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Task: ") + e.what());
    }
}

Task Task::find_by_id(pqxx::connection& conn, int task_id) {
    try {
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, title, description, EXTRACT(EPOCH FROM deadline)::bigint AS "
            "deadline_sec, "
            "status, priority, EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
            "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
            "FROM tasks WHERE id = $1",
            task_id);

        if (r.empty()) {
            throw std::runtime_error("Task not found");
        }

        const auto& row = r[0];
        Task task(row[0].as<int>(), row[1].as<int>(), row[2].as<std::string>(),
                  row[3].as<std::string>(), row[4].as<long>(), row[5].as<std::string>(),
                  row[6].as<int>(), row[7].as<long>(), row[8].as<long>());

        return task;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Task: ") + e.what());
    }
}