#pragma once

#include <db/connection_pool.hpp>

#include <string>

struct NewUser {
    std::string email;
    std::string name;
    std::string password_hash;
};

struct User {
    int id{};
    std::string email;
    std::string name;
    std::string password_hash;
    std::string created_at;
};

User insert_user(ConnectionPool& pool, const NewUser& user);