#include "db_config.hpp"

#include <cstdlib>
#include <stdexcept>
#include <utility>

static std::string get_env_or(const char* key, const char* def) {
    const char* v = std::getenv(key);
    if (v && *v)
        return std::string(v);
    return std::string(def);
}

static std::size_t get_env_size_or(const char* key, std::size_t def) {
    const char* v = std::getenv(key);
    if (!v || !*v)
        return def;
    try {
        std::size_t x = std::stoull(v);
        return x == 0 ? def : x;
    } catch (...) {
        return def;
    }
}

std::string DbConfig::connection_info() const {
    return "host=" + host + " port=" + port + " dbname=" + name + " user=" + user +
           " password=" + password;
}

DbConfig load_db_config_from_env() {
    DbConfig cfg;
    cfg.host = get_env_or("DB_HOST", "localhost");
    cfg.port = get_env_or("DB_PORT", "5432");
    cfg.name = get_env_or("DB_NAME", "postgres");
    cfg.user = get_env_or("DB_USER", "postgres");
    cfg.password = get_env_or("DB_PASSWORD", "postgres");
    cfg.pool_size = get_env_size_or("DB_POOL_SIZE", 10);
    return cfg;
}