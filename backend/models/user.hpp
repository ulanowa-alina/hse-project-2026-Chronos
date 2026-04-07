#ifndef USER_H
#define USER_H

#include <ctime>
#include <stdexcept>
#include <string>

class User {
  public:
    int id_;
    std::string email_;
    std::string name_;
    std::string status_;
    std::string password_hash_;
    std::time_t created_at_;

    User() = default;
    User(int id, std::string email, std::string name, std::string status, std::string password_hash,
         std::time_t created_at)
        : id_(id)
        , email_(std::move(email))
        , name_(std::move(name))
        , status_(std::move(status))
        , password_hash_(std::move(password_hash))
        , created_at_(created_at) {
        if (id_ < 0) {
            throw std::invalid_argument("User ID cannot be negative");
        }
        if (email_.find('@') == std::string::npos || email_.find('.') == std::string::npos) {
            throw std::invalid_argument("Invalid email format");
        }
        if (name_.empty() || name_.length() > 50) {
            throw std::invalid_argument("Name must be between 1 and 50 characters");
        }
        if (status_.empty()) {
            throw std::invalid_argument("Status cannot be empty");
        }
        if (password_hash_.empty()) {
            throw std::invalid_argument("Password hash cannot be empty");
        }
        if (created_at_ < 0) {
            throw std::invalid_argument("Created at cannot be negative");
        }
    }
};

#endif // USER_H
