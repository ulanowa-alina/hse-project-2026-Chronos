#include "board_repository.hpp"

#include <optional>
#include <stdexcept>

BoardRepository::BoardRepository(pqxx::connection& conn)
    : conn_(conn) {
}

void BoardRepository::insert(Board& board) {
    pqxx::work txn(conn_);

    pqxx::result r =
        txn.exec_params("INSERT INTO boards (user_id, title) VALUES ($1, $2) RETURNING id",
                        board.user_id_, board.title_);
    board.id_ = r[0][0].as<int>();

    txn.commit();
}

void BoardRepository::update(const Board& board) {
    pqxx::work txn(conn_);

    txn.exec_params("UPDATE boards SET user_id = $1, title = $2 WHERE id = $3", board.user_id_,
                    board.title_, board.id_);

    txn.commit();
}

void BoardRepository::save(Board& board) {
    try {
        if (board.id_ < 0) {
            throw std::domain_error(
                "Board ID cannot be negative (id: " + std::to_string(board.id_) + ")");
        }
        if (board.id_ == 0) {
            insert(board);
        } else {
            update(board);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Board: ") + e.what());
    }
}

std::optional<Board> BoardRepository::find_by_id(int board_id) {
    try {
        pqxx::work txn(conn_);
        pqxx::result r =
            txn.exec_params("SELECT id, user_id, title FROM boards WHERE id = $1", board_id);

        if (r.empty()) {
            return std::nullopt;
        }

        const auto& row = r[0];
        txn.commit();
        return Board(row[0].as<int>(), row[1].as<int>(), row[2].as<std::string>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Board: ") + e.what());
    }
}
