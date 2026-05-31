#include "s3_config.hpp"

#include <cstdlib>

namespace {

std::string get_env_or(const char* key, const char* def) {
    const char* value = std::getenv(key);
    if (value && *value) {
        return std::string(value);
    }
    return std::string(def);
}

bool get_env_bool_or(const char* key, bool def) {
    const char* value = std::getenv(key);
    if (!value || !*value) {
        return def;
    }

    const std::string str(value);
    return str == "1" || str == "true" || str == "TRUE";
}

} // namespace

S3Config load_s3_config_from_env() {
    S3Config cfg;
    cfg.bucket = get_env_or("S3_BUCKET", "");
    cfg.region = get_env_or("S3_REGION", "");
    cfg.endpoint = get_env_or("S3_ENDPOINT", "");
    cfg.access_key = get_env_or("S3_ACCESS_KEY", "");
    cfg.secret_key = get_env_or("S3_SECRET_KEY", "");
    cfg.use_path_style = get_env_bool_or("S3_USE_PATH_STYLE", false);
    return cfg;
}
