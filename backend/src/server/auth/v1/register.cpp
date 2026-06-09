#include "register.hpp"

#include "models/board.hpp"
#include "models/user.hpp"
#include "repositories/board_repository.hpp"
#include "repositories/user_repository.hpp"
#include "security/password_hashing.hpp"

#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sstream>

namespace auth::v1 {

namespace {

using nlohmann::json;
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

auto to_iso8601(std::time_t t) -> std::string {
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

auto build_create_response(const http::request<http::string_body>& req,
                           const User& created) -> http::response<http::string_body> {
    json out;
    out["id"] = created.id_;
    out["email"] = created.email_;
    out["name"] = created.name_;
    out["status"] = created.status_;
    out["avatar_s3_key"] = created.avatar_s3_key_;
    out["created_at"] = to_iso8601(created.created_at_);

    return build_json_response(req, http::status::ok, json{{"data", out}});
}

json parse_body(const http::request<http::string_body>& req) {
    return json::parse(req.body());
}

auto collect_missing_fields(const json& body) -> json {
    json missing = json::array();

    if (!body.contains("email")) {
        missing.push_back("email");
    }
    if (!body.contains("name")) {
        missing.push_back("name");
    }
    if (!body.contains("status")) {
        missing.push_back("status");
    }
    if (!body.contains("password")) {
        missing.push_back("password");
    }

    return missing;
}

std::string require_string_field(const json& body, const std::string& key) {
    if (!body.contains(key)) {
        throw std::invalid_argument("missing:" + key);
    }

    if (!body.at(key).is_string()) {
        throw std::invalid_argument("type:" + key);
    }

    return body.at(key).get<std::string>();
}

User parse_new_user(const json& body) {
    const std::string email = require_string_field(body, "email");
    const std::string name = require_string_field(body, "name");
    const std::string status = require_string_field(body, "status");
    const std::string password = require_string_field(body, "password");

    if (!user_validation::is_valid_email(email)) {
        throw std::invalid_argument("invalid_email");
    }

    if (name.empty() || name.size() > 50) {
        throw std::invalid_argument("invalid_name");
    }

    if (status.empty() || status.size() > 50) {
        throw std::invalid_argument("invalid_status");
    }

    if (password.empty()) {
        throw std::invalid_argument("empty_password");
    }

    if (password.size() < 8) {
        throw std::length_error("password_too_short");
    }

    const std::string password_hash = security::hash_password(password);
    return User(0, email, name, status, password_hash, "", std::time(nullptr));
}

User createUser(ConnectionPool& pool, const User& user) {
    UserRepository repo(pool);
    return repo.save(user);
}

void create_board(ConnectionPool& pool, int user_id) {
    BoardRepository repo(pool);
    const std::time_t now = std::time(nullptr);
    const Board b(0, user_id, "My board", "", now, now);
    (void) repo.save(b);
}
} // namespace

auto handleRegister(const http::request<http::string_body>& req,
                    ConnectionPool& pool) -> http::response<http::string_body> {
    try {
        spdlog::info("Register request received");

        const json body = parse_body(req);
        if (!body.is_object()) {
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid JSON format");
        }
        const json missing_fields = collect_missing_fields(body);

        if (!missing_fields.empty()) {
            spdlog::error("Request rejected: mising required fields");
            return build_api_error(req, http::status::bad_request, "MISSING_FIELD",
                                   "Missing required fields",
                                   json{{"missing_fields", missing_fields}});
        }

        const User new_user = parse_new_user(body);
        const User created = createUser(pool, new_user);
        create_board(pool, created.id_);

        spdlog::info("Register succeeded for user_id={}", created.id_);
        return build_create_response(req, created);
    } catch (const nlohmann::json::exception&) {
        spdlog::error("Request rejected: Invalid JSON format");
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid JSON format");

    } catch (const std::invalid_argument& e) {
        const std::string reason = e.what();

        if (reason == "type:email") {
            spdlog::error("Request rejected: Invalid email format");
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid email format", json{{"email", "Invalid email format"}});
        }

        if (reason == "type:name") {
            spdlog::error("Request rejected: Invalid name format");
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid name format", json{{"name", "Invalid name format"}});
        }

        if (reason == "type:status") {
            spdlog::error("Request rejected: Invalid status format");
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid status format",
                                   json{{"status", "Invalid status format"}});
        }

        if (reason == "type:password") {
            spdlog::error("Request rejected: Invalid password format");
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid password format",
                                   json{{"password", "Invalid password format"}});
        }

        if (reason == "invalid_email") {
            spdlog::error("Request rejected: Invalid email format");
            return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                                   "Validation failed", json{{"email", "Invalid email format"}});
        }

        if (reason == "invalid_name") {
            spdlog::error("Request rejected: Invalid name");
            return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                                   "Validation failed",
                                   json{{"name", "Name length must be between 1 and 50 symbols"}});
        }

        if (reason == "invalid_status") {
            spdlog::error("Request rejected: Invalid status");
            return build_api_error(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"status", "Status length must be between 1 and 50 symbols"}});
        }

        if (reason == "empty_password") {
            spdlog::error("Request rejected: Empty password's field");
            return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                                   "Validation failed",
                                   json{{"password", "Password cannot be empty"}});
        }

        spdlog::error("Request rejected: Invalid register credential: {}", reason);
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed");
    } catch (const std::length_error&) {
        spdlog::error("Request rejected: Password length is less then 8 simbols");
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"password", "Password length cannot be less than 8 symbols"}});
    } catch (const pqxx::sql_error& e) {
        const std::string msg = e.what();
        if (msg.find("users_email_key") != std::string::npos ||
            msg.find("duplicate key") != std::string::npos) {
            spdlog::error("Request rejected: User with this email already exists");
            return build_api_error(req, static_cast<http::status>(405), "EMAIL_ALREADY_EXISTS",
                                   "User with this email already exists",
                                   json{{"email", "already exists"}});
        }
        spdlog::error("Register failed with database error: {}", e.what());
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    } catch (const std::exception& e) {
        spdlog::error(" Register failed with unexpected error: {}", e.what());
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    }
}

} // namespace auth::v1
