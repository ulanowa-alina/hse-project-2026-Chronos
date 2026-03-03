#ifndef USER_H
#define USER_H

#include <optional>
#include <string>

class User {
  public:
    int id_;
    std::string email_;
    std::string name_;
    std::string password_hash_;

    User() = default;
    User(int id, const std::string& email, const std::string& name,
         const std::string& password_hash)
        : id_(id)
        , email_(email)
        , name_(name)
        , password_hash_(password_hash) {
    }
};

#endif // USER_H
