#pragma once

#include <string>

struct S3Config {
    std::string bucket;
    std::string region;
    std::string endpoint;
    std::string access_key;
    std::string secret_key;
    bool use_path_style = false;
};

S3Config load_s3_config_from_env();
