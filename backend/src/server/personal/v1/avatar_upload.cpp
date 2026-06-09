#include "avatar_upload.hpp"

#include "models/user.hpp"
#include "repositories/user_repository.hpp"
#include "server/utils/base64.hpp"
#include "storage/s3_config.hpp"
#include "storage/s3_uploader.hpp"
#include "security/password_hashing.hpp"

#include <array>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqxx/pqxx>
#include <string>

namespace http = boost::beast::http;

namespace personal::v1 {

namespace {

using nlohmann::json;

constexpr std::size_t kMaxAvatarBytes = 5U * 1024U * 1024U;
const std::array<std::string, 3> kAllowedContentTypes = {"image/jpeg", "image/png", "image/webp"};

struct ProfilePayload {
    std::string email;
    std::string name;
    std::string status;
    std::string password;
    bool has_password = false;
};

auto is_valid_name(const std::string& name) -> bool {
    return !name.empty() && name.size() <= 50;
}

auto build_json_response(const http::request<http::string_body>& req, http::status status,
                         const json& body) -> http::response<http::string_body> {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

auto build_api_error(const http::request<http::string_body>& req, http::status status,
                     const std::string& code, const std::string& message,
                     const json& details = nullptr) -> http::response<http::string_body> {
    json error_obj{{"code", code}, {"message", message}};
    if (!details.is_null()) {
        error_obj["details"] = details;
    }

    return build_json_response(req, status, json{{"error", error_obj}});
}

auto is_allowed_content_type(const std::string& content_type) -> bool {
    for (const auto& allowed : kAllowedContentTypes) {
        if (content_type == allowed) {
            return true;
        }
    }
    return false;
}

auto parse_json_body(const http::request<http::string_body>& req,
                     json& body) -> std::optional<http::response<http::string_body>> {
    try {
        body = json::parse(req.body());
    } catch (...) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid JSON format");
    }

    return std::nullopt;
}

auto validate_profile_payload(const http::request<http::string_body>& req, const json& body,
                              ProfilePayload& payload)
    -> std::optional<http::response<http::string_body>> {
    const bool has_email = body.contains("email");
    const bool has_name = body.contains("name");
    const bool has_status = body.contains("status");
    const bool has_password = body.contains("password");

    if (has_email && !body["email"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid email format", json{{"email", "Invalid email format"}});
    }
    if (has_name && !body["name"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid name format", json{{"name", "Invalid name format"}});
    }
    if (has_status && !body["status"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid status format", json{{"status", "Invalid status format"}});
    }
    if (has_password && !body["password"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid password format",
                               json{{"password", "Invalid password format"}});
    }

    json missing_fields = json::array();
    if (!has_email) {
        missing_fields.push_back("email");
    }
    if (!has_name) {
        missing_fields.push_back("name");
    }
    if (!has_status) {
        missing_fields.push_back("status");
    }

    if (!missing_fields.empty()) {
        return build_api_error(req, http::status::bad_request, "MISSING_FIELD",
                               "Missing required fields", json{{"missing_fields", missing_fields}});
    }

    payload.email = body["email"].get<std::string>();
    payload.name = body["name"].get<std::string>();
    payload.status = body["status"].get<std::string>();
    payload.has_password = has_password;
    payload.password = has_password ? body["password"].get<std::string>() : "";

    if (!user_validation::is_valid_email(payload.email)) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid email format", json{{"email", "Invalid email format"}});
    }
    if (!is_valid_name(payload.name)) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"name", "Name length must be between 1 and 50 symbols"}});
    }
    if (payload.status.empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed", json{{"status", "Status cannot be empty"}});
    }
    if (payload.has_password && payload.password.size() < 8) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"password", "Minimum length is 8 symbols"}});
    }

    return std::nullopt;
}

void apply_profile_payload(User& user, const ProfilePayload& payload) {
    user.email_ = payload.email;
    user.name_ = payload.name;
    user.status_ = payload.status;
    if (payload.has_password) {
        user.password_hash_ = security::hash_password(payload.password);
    }
}

} // namespace

auto handleAvatarUpload(const http::request<http::string_body>& req, ConnectionPool& pool,
                        int user_id) -> http::response<http::string_body> {
    json body;
    if (const auto parse_error = parse_json_body(req, body)) {
        return *parse_error;
    }

    const bool has_file_name = body.contains("file_name");
    const bool has_content_type = body.contains("content_type");
    const bool has_file_base64 = body.contains("file_base64");
    ProfilePayload profile_payload;

    if (has_file_name && !body["file_name"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid file_name format",
                               json{{"file_name", "Invalid file_name format"}});
    }
    if (has_content_type && !body["content_type"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid content_type format",
                               json{{"content_type", "Invalid content_type format"}});
    }
    if (has_file_base64 && !body["file_base64"].is_string()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid file_base64 format",
                               json{{"file_base64", "Invalid file_base64 format"}});
    }

    json missing_fields = json::array();
    if (!has_file_name) {
        missing_fields.push_back("file_name");
    }
    if (!has_content_type) {
        missing_fields.push_back("content_type");
    }
    if (!has_file_base64) {
        missing_fields.push_back("file_base64");
    }

    if (!missing_fields.empty()) {
        return build_api_error(req, http::status::bad_request, "MISSING_FIELD",
                               "Missing required fields", json{{"missing_fields", missing_fields}});
    }

    if (const auto profile_error = validate_profile_payload(req, body, profile_payload)) {
        return *profile_error;
    }

    const std::string file_name = body["file_name"].get<std::string>();
    const std::string content_type = body["content_type"].get<std::string>();
    const std::string file_base64 = body["file_base64"].get<std::string>();

    if (file_name.empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"file_name", "File name cannot be empty"}});
    }
    if (content_type.empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"content_type", "Content type cannot be empty"}});
    }
    if (file_base64.empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"file_base64", "File content cannot be empty"}});
    }
    if (!is_allowed_content_type(content_type)) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"content_type", "Unsupported image content type"}});
    }

    const auto decoded = server::utils::base64_decode(file_base64);
    if (!decoded.has_value()) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid base64 file payload",
                               json{{"file_base64", "Invalid base64 file payload"}});
    }
    if (decoded->empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"file_base64", "Decoded file content is empty"}});
    }
    if (decoded->size() > kMaxAvatarBytes) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"file_base64", "Avatar file exceeds 5 MB limit"}});
    }

    try {
        UserRepository repo(pool);
        const auto existing = repo.find_by_id(user_id);
        if (!existing.has_value()) {
            return build_api_error(req, http::status::not_found, "USER_NOT_FOUND",
                                   "User not found");
        }

        const std::string avatar_s3_key = "avatars/user_" + std::to_string(user_id) + "/avatar";
        User updated = *existing;
        apply_profile_payload(updated, profile_payload);
        updated.avatar_s3_key_ = avatar_s3_key;
        repo.save(updated);

        try {
            S3Uploader uploader(load_s3_config_from_env());
            uploader.upload_user_avatar(user_id, *decoded, content_type);
        } catch (const S3UploadError&) {
            try {
                repo.save(*existing);
            } catch (...) {
            }
            throw;
        }

        return build_json_response(req, http::status::ok,
                                   json{{"data", {{"avatar_s3_key", avatar_s3_key}}}});
    } catch (const pqxx::sql_error& e) {
        const std::string msg = e.what();
        if (msg.find("users_email_key") != std::string::npos ||
            msg.find("duplicate key") != std::string::npos) {
            return build_api_error(req, static_cast<http::status>(405), "EMAIL_ALREADY_EXISTS",
                                   "User with this email already exists",
                                   json{{"email", "already exists"}});
        }

        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    } catch (const S3UploadError&) {
        return build_api_error(req, http::status::internal_server_error, "S3_UPLOAD_ERROR",
                               "Failed to upload avatar to S3");
    } catch (const std::exception&) {
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    }
}

auto handleAvatarDelete(const http::request<http::string_body>& req, ConnectionPool& pool,
                        int user_id) -> http::response<http::string_body> {
    json body;
    if (const auto parse_error = parse_json_body(req, body)) {
        return *parse_error;
    }

    ProfilePayload profile_payload;
    if (const auto profile_error = validate_profile_payload(req, body, profile_payload)) {
        return *profile_error;
    }

    try {
        UserRepository repo(pool);
        const auto existing = repo.find_by_id(user_id);
        if (!existing.has_value()) {
            return build_api_error(req, http::status::not_found, "USER_NOT_FOUND",
                                   "User not found");
        }

        User updated = *existing;
        apply_profile_payload(updated, profile_payload);
        updated.avatar_s3_key_.clear();
        repo.save(updated);

        return build_json_response(req, http::status::ok, json{{"data", {{"avatar_s3_key", ""}}}});
    } catch (const pqxx::sql_error& e) {
        const std::string msg = e.what();
        if (msg.find("users_email_key") != std::string::npos ||
            msg.find("duplicate key") != std::string::npos) {
            return build_api_error(req, static_cast<http::status>(405), "EMAIL_ALREADY_EXISTS",
                                   "User with this email already exists",
                                   json{{"email", "already exists"}});
        }

        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    } catch (const std::exception&) {
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    }
}

} // namespace personal::v1
