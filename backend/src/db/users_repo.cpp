#include "users_repo.hpp"

#include <db/connection_pool.hpp>
#include <pqxx/pqxx>
#include <stdexcept>

User insert_user(ConnectionPool& pool, const NewUser& user) {
    auto h = pool.acquire();
    pqxx::work tx(h.conn());

    pqxx::result r = tx.exec_params(
       "INSERT INTO users (email, name, password_hash) "
       "VALUES ($1, $2, $3) "
       "RETURNING id, email, name, password_hash, created_at",
       user.email, user.name, user.password_hash
   );

    tx.commit();

    if (r.empty()) {
        throw std::runtime_error{ "Unable to insert user" };
    }

    User out;
    out.id = r[0][0].as<int>();
    out.email = r[0][1].as<std::string>();
    out.name = r[0][2].as<std::string>();
    out.password_hash = r[0][3].as<std::string>();
    out.created_at = r[0][4].c_str();
    return out;
}