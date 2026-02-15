#include "user.hpp"
#include <stdexcept>

void User::save(pqxx::connection &conn) {
    try {
        pqxx::work txn(conn);

        if (id == 0) {
            pqxx::result r = txn.exec(
                "INSERT INTO users (email, name, password_hash) VALUES (" +
                txn.quote(email) + ", " +
                txn.quote(name) + ", " +
                txn.quote(password_hash) +
                ") RETURNING id"
            );

            id = r[0][0].as<int>();
        } else {
            txn.exec0(
                "UPDATE users SET "
                "email = " + txn.quote(email) + ", " +
                "name = " + txn.quote(name) + ", " +
                "password_hash = " + txn.quote(password_hash) +
                " WHERE id = " + txn.quote(id)
            );
        }

        txn.commit();
    }
    catch (const std::exception &e) {
        throw std::runtime_error(std::string("User::save failed: ") + e.what());
    }
}

User User::find_by_id(pqxx::connection &conn, int id) {
    try {
        pqxx::work txn(conn);

        pqxx::result r = txn.exec(
            "SELECT id, email, name, password_hash FROM users WHERE id = " +
            txn.quote(id)
        );

        txn.commit();

        if (r.empty()) {
            throw std::runtime_error("User not found");
        }

        return User(
            r[0]["id"].as<int>(),
            r[0]["email"].as<std::string>(),
            r[0]["name"].as<std::string>(),
            r[0]["password_hash"].as<std::string>()
        );
    }
    catch (const std::exception &e) {
        throw std::runtime_error(std::string("User::find_by_id failed: ") + e.what());
    }
}
