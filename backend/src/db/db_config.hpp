#pragma once

#include <cstddef>
#include <string>

struct DbConfig {
    std::string host;
    std::string port;
    std::string name;
    std::string user;
    std::string password;
    std::size_t pool_size = 10;

    std::string connection_info() const;
};

DbConfig load_db_config_from_env();