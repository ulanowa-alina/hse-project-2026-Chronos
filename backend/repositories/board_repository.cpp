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
                        "RETURNING id, EXTRACT(EPOCH FROM created_at)::bigint",
                        board.user_id_, board.title_, board.description_, board.is_private_);

    txn.commit();

    return Board(r[0][0].as<int>(), board.user_id_, board.title_, board.description_,
                 board.is_private_, static_cast<std::time_t>(r[0][1].as<long>()),
                 static_cast<std::time_t>(r[0][1].as<long>()));
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
        if (board.id_ < 0) {
            throw std::domain_error(
                "Board ID cannot be negative (id: " + std::to_string(board.id_) + ")");
        }
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
