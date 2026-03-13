#ifndef USER_REPOSITORY_HPP
#define USER_REPOSITORY_HPP

#include "../models/user.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <vector>

class UserRepository {
  public:
    explicit UserRepository(pqxx::connection& conn);

    void save(User& user);
    std::optional<User> find_by_id(int user_id);

  private:
    pqxx::connection& conn_;

    void insert(User& user);
    void update(const User& user);
};

#endif // USER_REPOSITORY_HPP
