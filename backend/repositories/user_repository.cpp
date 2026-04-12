#include "user_repository.hpp"

#include <optional>
#include <stdexcept>

UserRepository::UserRepository(ConnectionPool& pool)
    : pool_(pool) {
}

User UserRepository::insert(const User& user) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r = txn.exec_params(
        "INSERT INTO users (email, name, status, password_hash) VALUES ($1, $2, $3, $4) "
        "RETURNING id, EXTRACT(EPOCH FROM created_at)::bigint AS created_sec",
        user.email_, user.name_, user.status_, user.password_hash_);

    txn.commit();
    return User(r[0][0].as<int>(), user.email_, user.name_, user.status_, user.password_hash_,
                static_cast<std::time_t>(r[0]["created_sec"].as<long>()));
}

void UserRepository::update(const User& user) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    txn.exec_params("UPDATE users SET email = $1, name = $2, status = $3, password_hash = $4 "
                    "WHERE id = $5",
                    user.email_, user.name_, user.status_, user.password_hash_, user.id_);
    txn.commit();
}

User UserRepository::save(const User& user) {
    if (user.id_ == 0) {
        return insert(user);
    }
    update(user);
    return user;
}

std::optional<User> UserRepository::find_by_id(int user_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, email, name, status, password_hash, "
                                         "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec "
                                         "FROM users WHERE id = $1",
                                         user_id);

        if (r.empty()) {
            return std::nullopt;
        }

        txn.commit();
        const auto& row = r[0];
        return User(row["id"].as<int>(), row["email"].as<std::string>(),
                    row["name"].as<std::string>(), row["status"].as<std::string>(),
                    row["password_hash"].as<std::string>(), row["created_sec"].as<long>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("User::find_by_id failed: ") + e.what());
    }
}

std::optional<User> UserRepository::find_by_email(const std::string& email) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params("SELECT id, email, name, status, password_hash, "
                                         "EXTRACT(EPOCH FROM created_at)::bigint AS created_sec "
                                         "FROM users WHERE email = $1",
                                         email);

        if (r.empty()) {
            return std::nullopt;
        }

        txn.commit();
        const auto& row = r[0];
        return User(row["id"].as<int>(), row["email"].as<std::string>(),
                    row["name"].as<std::string>(), row["status"].as<std::string>(),
                    row["password_hash"].as<std::string>(), row["created_sec"].as<long>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("User::find_by_email failed: ") + e.what());
    }
}
