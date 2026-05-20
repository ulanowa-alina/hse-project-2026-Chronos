#include "login.hpp"

#include "repositories/user_repository.hpp"
#include "server/auth/jwt.hpp"

#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <sstream>
#include <string>

namespace auth::v1 {

namespace {

using nlohmann::json;

auto is_valid_email(const std::string& email) -> bool {
    if (email.empty()) {
        return false;
    }

    if (email.size() > 254) {
        return false;
    }

    if (email.find(' ') != std::string::npos) {
        return false;
    }

    const auto at_pos = email.find('@');
    if (at_pos == std::string::npos) {
        return false;
    }

    if (at_pos == 0 || at_pos != email.rfind('@')) {
        return false;
    }

    const std::string local = email.substr(0, at_pos);
    const std::string domain = email.substr(at_pos + 1);

    if (local.empty() || domain.empty()) {
        return false;
    }

    if (local.front() == '.' || local.back() == '.') {
        return false;
    }

    if (domain.front() == '.' || domain.back() == '.') {
        return false;
    }

    if (email.find("..") != std::string::npos) {
        return false;
    }

    if (domain.find('.') == std::string::npos) {
        return false;
    }

    for (char ch : email) {
        const bool is_ascii_letter = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        const bool is_digit = ch >= '0' && ch <= '9';
        const bool is_allowed_symbol =
            ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+';

        if (!is_ascii_letter && !is_digit && !is_allowed_symbol) {
            return false;
        }
    }

    return true;
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

struct LoginRequest {
    std::string email;
    std::string password;
};

auto parse_login_request(const http::request<http::string_body>& req, LoginRequest& out,
                         http::response<http::string_body>& error_response) -> bool {
    json body;
    try {
        body = json::parse(req.body());
    } catch (...) {
        error_response = build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                         "Invalid JSON format");
        return false;
    }

    if (!body.is_object()) {
        error_response = build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                         "Invalid JSON format");
        return false;
    }

    const bool has_email = body.contains("email");
    const bool has_password = body.contains("password");

    if (has_email && !body["email"].is_string()) {
        error_response =
            build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                            "Invalid email format", json{{"email", "Invalid email format"}});
        return false;
    }

    if (has_password && !body["password"].is_string()) {
        error_response = build_api_error(req, http::status::bad_request, "INVALID_FORMAT",
                                         "Invalid password format",
                                         json{{"password", "Invalid password format"}});
        return false;
    }

    json missing_fields = json::array();
    if (!has_email) {
        missing_fields.push_back("email");
    }
    if (!has_password) {
        missing_fields.push_back("password");
    }

    if (!missing_fields.empty()) {
        error_response =
            build_api_error(req, http::status::bad_request, "MISSING_FIELD",
                            "Missing required fields", json{{"missing_fields", missing_fields}});
        return false;
    }

    out.email = body["email"].get<std::string>();
    out.password = body["password"].get<std::string>();

    if (!is_valid_email(out.email)) {
        error_response =
            build_api_error(req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                            json{{"email", "Invalid email format"}});
        return false;
    }

    if (out.password.empty()) {
        error_response =
            build_api_error(req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                            json{{"password", "Password cannot be empty"}});
        return false;
    }

    if (out.password.size() < 8) {
        error_response =
            build_api_error(req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                            json{{"password", "Password length cannot be less than 8 symbols"}});
        return false;
    }

    return true;
}

auto build_password_hash(const std::string& password) -> std::string {
    return "hash:" + password;
}

auto is_password_valid(const User& user, const std::string& password) -> bool {
    return user.password_hash_ == build_password_hash(password);
}

auto to_iso8601(std::time_t t) -> std::string {
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

auto build_login_success_response(const http::request<http::string_body>& req, const User& user,
                                  const std::string& token) -> http::response<http::string_body> {
    json user_json{{"id", user.id_},
                   {"email", user.email_},
                   {"name", user.name_},
                   {"status", user.status_},
                   {"created_at", to_iso8601(user.created_at_)}};

    json out{{"token", token}, {"user", user_json}};

    return build_json_response(req, http::status::ok, json{{"data", out}});
}

} // namespace

auto handleLogin(const http::request<http::string_body>& req,
                 ConnectionPool& pool) -> http::response<http::string_body> {
    http::response<http::string_body> error_response{http::status::bad_request, req.version()};
    LoginRequest login_request;

    if (!parse_login_request(req, login_request, error_response)) {
        return error_response;
    }

    try {
        UserRepository repo(pool);
        const auto user = repo.find_by_email(login_request.email);

        if (!user.has_value() || !is_password_valid(*user, login_request.password)) {
            return build_api_error(req, http::status::unauthorized, "UNAUTHORIZED",
                                   "Invalid email or password");
        }

        const std::string token = auth::create_token(user->id_);
        return build_login_success_response(req, *user, token);
    } catch (const pqxx::sql_error&) {
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    } catch (const std::exception&) {
        return build_api_error(req, http::status::internal_server_error, "DATABASE_ERROR",
                               "Database error");
    }
}

} // namespace auth::v1
