#ifndef USER_H
#define USER_H

#include <ctime>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace user_validation {

inline auto is_ascii_letter(const char ch) -> bool {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

inline auto is_digit(const char ch) -> bool {
    return ch >= '0' && ch <= '9';
}

inline auto is_ascii_alnum(const char ch) -> bool {
    return is_ascii_letter(ch) || is_digit(ch);
}

inline auto is_valid_domain_label(const std::string& label) -> bool {
    if (label.empty() || label.size() > 63) {
        return false;
    }

    if (!is_ascii_alnum(label.front()) || !is_ascii_alnum(label.back())) {
        return false;
    }

    for (const char ch : label) {
        if (!is_ascii_alnum(ch) && ch != '-') {
            return false;
        }
    }

    return true;
}

inline auto is_valid_email(const std::string& email) -> bool {
    if (email.empty() || email.size() > 254) {
        return false;
    }

    if (email.find(' ') != std::string::npos) {
        return false;
    }

    const auto at_pos = email.find('@');
    if (at_pos == std::string::npos) {
        return false;
    }

    if (at_pos == 0 || at_pos != email.rfind('@')) {
        return false;
    }

    const std::string local = email.substr(0, at_pos);
    const std::string domain = email.substr(at_pos + 1);

    if (local.empty() || local.size() > 64 || domain.empty()) {
        return false;
    }

    if (local.front() == '.' || local.back() == '.') {
        return false;
    }

    if (domain.front() == '.' || domain.back() == '.') {
        return false;
    }

    if (email.find("..") != std::string::npos) {
        return false;
    }

    if (domain.find('.') == std::string::npos) {
        return false;
    }

    for (const char ch : local) {
        const bool is_allowed_symbol = ch == '.' || ch == '_' || ch == '-' || ch == '+';

        if (!is_ascii_alnum(ch) && !is_allowed_symbol) {
            return false;
        }
    }

    std::size_t label_start = 0;
    std::size_t dot_pos = domain.find('.');
    while (dot_pos != std::string::npos) {
        if (!is_valid_domain_label(domain.substr(label_start, dot_pos - label_start))) {
            return false;
        }

        label_start = dot_pos + 1;
        dot_pos = domain.find('.', label_start);
    }

    const std::string tld = domain.substr(label_start);
    if (!is_valid_domain_label(tld) || tld.size() < 2) {
        return false;
    }

    bool tld_has_letter = false;
    for (const char ch : tld) {
        if (is_ascii_letter(ch)) {
            tld_has_letter = true;
            break;
        }
    }

    return tld_has_letter;
}

} // namespace user_validation

class User {
  public:
    int id_;
    std::string email_;
    std::string name_;
    std::string status_;
    std::string password_hash_;
    std::string avatar_s3_key_;
    std::time_t created_at_;

    User() = default;
    User(int id, std::string email, std::string name, std::string status, std::string password_hash,
         std::string avatar_s3_key, std::time_t created_at)
        : id_(id)
        , email_(std::move(email))
        , name_(std::move(name))
        , status_(std::move(status))
        , password_hash_(std::move(password_hash))
        , avatar_s3_key_(std::move(avatar_s3_key))
        , created_at_(created_at) {
        if (id_ < 0) {
            throw std::invalid_argument("User ID cannot be negative");
        }
        if (!user_validation::is_valid_email(email_)) {
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
