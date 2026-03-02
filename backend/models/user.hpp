#ifndef USER_H
#define USER_H

#include <string>
#include <pqxx/pqxx>
#include <optional>

class User {
public:
    int id_;
    std::string email_;
    std::string name_;
    std::string password_hash_;

    User() = default;
    User(int id, const std::string& email, const std::string& name, const std::string& password_hash);

    void save(pqxx::connection &conn);
    static std::optional<User> find_by_id(pqxx::connection &conn, int id);
};

#endif // USER_H
