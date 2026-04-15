#include "status_repository.hpp"

#include <stdexcept>

StatusRepository::StatusRepository(ConnectionPool& pool)
    : pool_(pool) {
}

Status StatusRepository::create(int board_id, const std::string& name, int position) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r =
        txn.exec_params("INSERT INTO statuses (board_id, name, position) VALUES ($1, $2, $3) "
                        "RETURNING id, board_id, name, position",
                        board_id, name, position);

    txn.commit();

    if (r.empty()) {
        throw std::runtime_error("Failed to create status");
    }

    const auto& row = r[0];
    return Status(row["id"].as<int>(), row["board_id"].as<int>(), row["name"].as<std::string>(),
                  row["position"].as<int>());
}

void StatusRepository::create_defaults_for_board(int board_id) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    txn.exec_params("INSERT INTO statuses (board_id, name, position) VALUES "
                    "($1, 'todo', 0), "
                    "($1, 'in_progress', 1), "
                    "($1, 'done', 2) "
                    "ON CONFLICT (board_id, name) DO NOTHING",
                    board_id);

    txn.commit();
}

std::optional<Status> StatusRepository::find_by_board_and_name(int board_id,
                                                               const std::string& name) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, name, position FROM statuses WHERE board_id = $1 AND name = $2",
            board_id, name);

        if (r.empty()) {
            return std::nullopt;
        }

        txn.commit();
        const auto& row = r[0];
        return Status(row["id"].as<int>(), row["board_id"].as<int>(), row["name"].as<std::string>(),
                      row["position"].as<int>());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Status::find_by_board_and_name failed: ") + e.what());
    }
}

std::optional<Status> StatusRepository::find_by_id(int status_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params(
            "SELECT id, board_id, name, position FROM statuses WHERE id = $1", status_id);

        if (r.empty()) {
            return std::nullopt;
        }

        txn.commit();
        const auto& row = r[0];
        return Status(row["id"].as<int>(), row["board_id"].as<int>(), row["name"].as<std::string>(),
                      row["position"].as<int>());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Status::find_by_id failed: ") + e.what());
    }
}

std::vector<Status> StatusRepository::find_by_board_id(int board_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, board_id, name, position FROM statuses "
                                         "WHERE board_id = $1 ORDER BY position ASC, id ASC",
                                         board_id);

        std::vector<Status> statuses;
        statuses.reserve(r.size());
        for (const auto& row : r) {
            statuses.emplace_back(row["id"].as<int>(), row["board_id"].as<int>(),
                                  row["name"].as<std::string>(), row["position"].as<int>());
        }

        txn.commit();
        return statuses;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Status::find_by_board_id failed: ") + e.what());
    }
}

bool StatusRepository::delete_by_id(int status_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r =
            txn.exec_params("DELETE FROM statuses WHERE id = $1 RETURNING id", status_id);

        txn.commit();
        return !r.empty();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Status::delete_by_id failed: ") + e.what());
    }
}
