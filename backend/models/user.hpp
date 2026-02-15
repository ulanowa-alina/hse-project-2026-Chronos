#ifndef USER_H
#define USER_H

#include <string>
#include <pqxx/pqxx>

class User {
public:
    int id;
    std::string email;
    std::string name;
    std::string password_hash;

    User() = default;
    User(int _id, const std::string& _email, const std::string& _name, const std::string& _password_hash)
        : id(_id), email(_email), name(_name), password_hash(_password_hash) {}

    void save(pqxx::connection &conn);
    static User find_by_id(pqxx::connection &conn, int id);
};

#endif // USER_H
