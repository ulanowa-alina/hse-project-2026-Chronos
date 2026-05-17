#include "board_repository.hpp"

#include <optional>
#include <stdexcept>

BoardRepository::BoardRepository(ConnectionPool& pool)
    : pool_(pool) {
}

Board BoardRepository::insert(const Board& board) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r =
        txn.exec_params("INSERT INTO boards (user_id, title, description, is_private) "
                        "VALUES ($1, $2, $3, $4) "
                        "RETURNING id, "
                        "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
                        "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec",
                        board.user_id_, board.title_, board.description_, board.is_private_);

    const int board_id = r[0][0].as<int>();
    txn.exec_params("INSERT INTO statuses (board_id, name, position) VALUES "
                    "($1, 'todo', 0), "
                    "($1, 'in_progress', 1), "
                    "($1, 'done', 2)",
                    board_id);

    txn.commit();

    return Board(board_id, board.user_id_, board.title_, board.description_, board.is_private_,
                 static_cast<std::time_t>(r[0]["created_sec"].as<long>()),
                 static_cast<std::time_t>(r[0]["updated_sec"].as<long>()));
}

Board BoardRepository::update(const Board& board) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r = txn.exec_params("UPDATE boards SET user_id = $1, title = $2, description = "
                                     "$3, is_private = $4, updated_at = NOW() "
                                     "WHERE id = $5 "
                                     "RETURNING EXTRACT(EPOCH FROM updated_at)::bigint",
                                     board.user_id_, board.title_, board.description_,
                                     board.is_private_, board.id_);

    txn.commit();

    return Board(board.id_, board.user_id_, board.title_, board.description_, board.is_private_,
                 board.created_at_, static_cast<std::time_t>(r[0][0].as<long>()));
}

Board BoardRepository::save(const Board& board) {
    try {
        if (board.id_ == 0) {
            return insert(board);
        } else {
            return update(board);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Board: ") + e.what());
    }
}

std::optional<Board> BoardRepository::find_by_id(int board_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, user_id, title, description, is_private, "
                                         "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
                                         "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
                                         "FROM boards WHERE id = $1",
                                         board_id);

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        txn.commit();
        return Board(row["id"].as<int>(), row["user_id"].as<int>(), row["title"].as<std::string>(),
                     row["description"].is_null() ? "" : row["description"].as<std::string>(),
                     row["is_private"].as<bool>(), row["created_sec"].as<long>(),
                     row["updated_sec"].is_null() ? row["created_sec"].as<long>()
                                                  : row["updated_sec"].as<long>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Board: ") + e.what());
    }
}

std::optional<Board> BoardRepository::find_by_user_id(int user_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, user_id, title, description, is_private, "
                                         "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
                                         "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
                                         "FROM boards WHERE user_id = $1 ORDER BY id ASC LIMIT 1",
                                         user_id);

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        txn.commit();
        return Board(row["id"].as<int>(), row["user_id"].as<int>(), row["title"].as<std::string>(),
                     row["description"].is_null() ? "" : row["description"].as<std::string>(),
                     row["is_private"].as<bool>(), row["created_sec"].as<long>(),
                     row["updated_sec"].is_null() ? row["created_sec"].as<long>()
                                                  : row["updated_sec"].as<long>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Board by user id: ") + e.what());
    }
}

std::vector<Board> BoardRepository::find_all_by_user_id(int user_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, user_id, title, description, is_private, "
                                         "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec, "
                                         "EXTRACT(EPOCH FROM updated_at)::bigint AS updated_sec "
                                         "FROM boards WHERE user_id = $1 ORDER BY id ASC",
                                         user_id);

        std::vector<Board> boards;
        boards.reserve(r.size());
        for (const auto& row : r) {
            boards.emplace_back(
                row["id"].as<int>(), row["user_id"].as<int>(), row["title"].as<std::string>(),
                row["description"].is_null() ? "" : row["description"].as<std::string>(),
                row["is_private"].as<bool>(), row["created_sec"].as<long>(),
                row["updated_sec"].is_null() ? row["created_sec"].as<long>()
                                             : row["updated_sec"].as<long>());
        }

        txn.commit();
        return boards;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Boards by user id: ") + e.what());
    }
}

bool BoardRepository::delete_by_id(int board_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        const pqxx::result r =
            txn.exec_params("DELETE FROM boards WHERE id = $1 RETURNING id", board_id);

        txn.commit();
        return !r.empty();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to delete Board: ") + e.what());
    }
}
