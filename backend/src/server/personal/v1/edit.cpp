#include "edit.hpp"

#include "models/user.hpp"
#include "repositories/user_repository.hpp"

#include <boost/beast/http.hpp>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <sstream>
#include <string>

namespace http = boost::beast::http;

namespace personal::v1 {

namespace {

using nlohmann::json;

auto is_valid_email(const std::string& email) -> bool {
    if (email.empty() || email.size() > 255) {
        return false;
    }

    if (email.find(' ') != std::string::npos) {
        return false;
    }

    const auto at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0 || at_pos + 1 >= email.size()) {
        return false;
    }

    const auto dot_pos = email.find('.', at_pos + 1);
    if (dot_pos == std::string::npos || dot_pos == at_pos + 1 || dot_pos + 1 >= email.size()) {
        return false;
    }

    return true;
}

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

auto to_iso8601(std::time_t t) -> std::string {
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

auto build_update_response(const http::request<http::string_body>& req,
                           const User& user) -> http::response<http::string_body> {
    json out{{"id", user.id_},
             {"email", user.email_},
             {"name", user.name_},
             {"status", user.status_},
             {"created_at", to_iso8601(user.created_at_)}};

    return build_json_response(req, http::status::ok, json{{"data", out}});
}

} // namespace

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body> {
    json body;
    try {
        body = json::parse(req.body());
    } catch (...) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid JSON format");
    }

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

    if (!has_name) {
        missing_fields.push_back("name");
    }
    if (!has_email) {
        missing_fields.push_back("email");
    }
    if (!has_status) {
        missing_fields.push_back("status");
    }
    if (!has_password) {
        missing_fields.push_back("password");
    }

    if (!missing_fields.empty()) {
        return build_api_error(req, http::status::bad_request, "MISSING_FIELD",
                               "Missing required fields", json{{"missing_fields", missing_fields}});
    }

    const std::string email = has_email ? body["email"].get<std::string>() : "";
    const std::string name = has_name ? body["name"].get<std::string>() : "";
    const std::string status = has_status ? body["status"].get<std::string>() : "";
    const std::string password = has_password ? body["password"].get<std::string>() : "";

    if (has_email && !is_valid_email(email)) {
        return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                               "Invalid email format", json{{"email", "Invalid email format"}});
    }

    if (has_name && !is_valid_name(name)) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"name", "Name length must be between 1 and 50 symbols"}});
    }

    if (status.empty()) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed", json{{"status", "Status cannot be empty"}});
    }

    if (has_password && password.size() < 8) {
        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed",
                               json{{"password", "Minimum length is 8 symbols"}});
    }

    const std::string password_hash = has_password ? ("hash:" + password) : "";

    try {
        UserRepository repo(pool);

        const auto existing = repo.find_by_id(user_id);

        if (!existing.has_value()) {
            return build_api_error(req, static_cast<http::status>(404), "USER_NOT_FOUND",
                                   "User not found");
        }

        User updated = *existing;

        if (has_email) {
            updated.email_ = email;
        }
        if (has_name) {
            updated.name_ = name;
        }
        if (has_status) {
            updated.status_ = status;
        }
        if (has_password) {
            updated.password_hash_ = password_hash;
        }

        const User saved = repo.save(updated);
        return build_update_response(req, saved);

    } catch (const std::invalid_argument& e) {
        const std::string reason = e.what();

        if (reason == "Invalid email format") {
            return build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                   "Invalid email format", json{{"email", "Invalid email format"}});
        }

        return build_api_error(req, http::status::bad_request, "VALIDATION_ERROR",
                               "Validation failed", json{{"validation", reason}});
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
