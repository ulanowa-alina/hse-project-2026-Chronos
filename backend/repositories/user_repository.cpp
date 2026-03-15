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
        "INSERT INTO users (email, name, password_hash) VALUES ($1, $2, $3) RETURNING id",
        user.email_, user.name_, user.password_hash_);

    txn.commit();
    return User(r[0][0].as<int>(), user.email_, user.name_, user.password_hash_);
}

void UserRepository::update(const User& user) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    txn.exec_params("UPDATE users SET email = $1, name = $2, password_hash = $3 WHERE id = $4",
                    user.email_, user.name_, user.password_hash_, user.id_);
    txn.commit();
}

User UserRepository::save(const User& user) {
    try {
        if (user.id_ < 0) {
            throw std::domain_error("User ID cannot be negative (id: " + std::to_string(user.id_) +
                                    ")");
        }
        if (user.id_ == 0) {
            return insert(user);
        } else {
            update(user);
            return user;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("User::save failed: ") + e.what());
    }
}

std::optional<User> UserRepository::find_by_id(int user_id) {
    try {
        auto handle = pool_.acquire();
        pqxx::work txn(handle.conn());

        pqxx::result r = txn.exec_params(
            "SELECT id, email, name, password_hash FROM users WHERE id = $1", user_id);

        if (r.empty()) {
            return std::nullopt;
        }

        txn.commit();
        const auto& row = r[0];
        return User(row["id"].as<int>(), row["email"].as<std::string>(),
                    row["name"].as<std::string>(), row["password_hash"].as<std::string>());

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("User::find_by_id failed: ") + e.what());
    }
}
