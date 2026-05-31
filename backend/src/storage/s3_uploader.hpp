#pragma once

#include "s3_config.hpp"

#include <stdexcept>
#include <string>

class S3UploadError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

class S3Uploader {
  public:
    explicit S3Uploader(S3Config config);

    auto upload_user_avatar(int user_id, const std::string& payload,
                            const std::string& content_type) const -> std::string;
    void delete_object(const std::string& object_key) const;

  private:
    S3Config config_;
};
