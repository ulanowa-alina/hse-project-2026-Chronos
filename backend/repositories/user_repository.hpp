#ifndef USER_REPOSITORY_HPP
#define USER_REPOSITORY_HPP

#include "../models/user.hpp"
#include "../src/db/connection_pool.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

class UserRepository {
  public:
    explicit UserRepository(ConnectionPool& pool);

    User save(const User& user);
    std::optional<User> find_by_id(int user_id);
    std::optional<User> find_by_email(const std::string& email);

  private:
    ConnectionPool& pool_;

    User insert(const User& user);
    void update(const User& user);
};

#endif // USER_REPOSITORY_HPP
