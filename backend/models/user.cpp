#include "user.hpp"
#include <stdexcept>

User::User(int id, const std::string& email, const std::string& name, const std::string& password_hash)
    : id_(id)
    , email_(email)
    , name_(name),
    password_hash_(password_hash)
{}

void User::save(pqxx::connection &conn) {
    try {
        pqxx::work txn(conn);

        if (id_ == 0) {
            pqxx::result r = txn.exec(
                "INSERT INTO users (email, name, password_hash) VALUES (" +
                txn.quote(email_) + ", " +
                txn.quote(name_) + ", " +
                txn.quote(password_hash_) +
                ") RETURNING id"
            );

            id_ = r[0][0].as<int>();
        } else {
            txn.exec0(
                "UPDATE users SET "
                "email = " + txn.quote(email_) + ", " +
                "name = " + txn.quote(name_) + ", " +
                "password_hash = " + txn.quote(password_hash_) +
                " WHERE id = " + txn.quote(id_)
            );
        }

        txn.commit();
    }
    catch (const std::exception &e) {
        throw std::runtime_error(std::string("User::save failed: ") + e.what());
    }
}

std::optional<User> User::find_by_id(pqxx::connection &conn, int id) {
    try {
        pqxx::work txn(conn);

        pqxx::result r = txn.exec(
            "SELECT id, email, name, password_hash FROM users WHERE id = " +
            txn.quote(id)
        );

        txn.commit();

        if (r.empty()) {
            return std::nullopt;
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
