#include "board.hpp"

#include <stdexcept>

void Board::save(pqxx::connection& conn) {
    try {
        pqxx::work txn(conn);

        if (id == 0) {
            pqxx::result r = txn.exec_params(
                "INSERT INTO boards (user_id, title) VALUES ($1, $2) RETURNING id", user_id, title);
            id = r[0][0].as<int>();
        } else {
            txn.exec_params("UPDATE boards SET user_id = $1, title = $2 WHERE id = $3", user_id,
                            title, id);
        }

        txn.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to save Board: ") + e.what());
    }
}

Board Board::find_by_id(pqxx::connection& conn, int board_id) {
    try {
        pqxx::work txn(conn);
        pqxx::result r =
            txn.exec_params("SELECT id, user_id, title FROM boards WHERE id = $1", board_id);

        if (r.empty()) {
            throw std::runtime_error("Board not found");
        }

        const auto& row = r[0];
        Board board(row[0].as<int>(), row[1].as<int>(), row[2].as<std::string>());
        return board;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to find Board: ") + e.what());
    }
}