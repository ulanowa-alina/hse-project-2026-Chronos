#include "info.hpp"

#include "repositories/user_repository.hpp"
#include "utils/response_utils.hpp"

#include <boost/beast/http.hpp>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

namespace http = boost::beast::http;

namespace personal::v1 {

namespace {
auto build_error_response(const http::request<http::string_body>& req,
                          const std::exception&) -> http::response<http::string_body> {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = R"({"error":{"code":"DATABASE_ERROR","message":"Database error"}})";
    res.prepare_payload();
    return res;
}

auto to_iso8601(std::time_t t) -> std::string {
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

auto build_ok_response(const http::request<http::string_body>& req,
                       const User& user) -> http::response<http::string_body> {
    nlohmann::json out{{"id", user.id_},
                       {"email", user.email_},
                       {"name", user.name_},
                       {"status", user.status_},
                       {"created_at", to_iso8601(user.created_at_)}};

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = nlohmann::json{{"data", out}}.dump();
    res.prepare_payload();
    return res;
}

auto build_not_found_response(const http::request<http::string_body>& req)
    -> http::response<http::string_body> {
    http::response<http::string_body> res{static_cast<http::status>(404), req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = R"({"error":{"code":"USER_NOT_FOUND","message":"User not found"}})";
    res.prepare_payload();
    return res;
}
} // namespace

auto handleInfo(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body> {
    spdlog::info("User get request received");
    if (req.method() != http::verb::get) {
        spdlog::warn("Board get rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    try {
        UserRepository repo(pool);
        const auto user = repo.find_by_id(user_id);

        if (!user.has_value()) {
            return build_not_found_response(req);
        }

        spdlog::info("Successfully get user info for user_id={}", user_id);
        return build_ok_response(req, *user);
    } catch (const std::runtime_error& e) {
        spdlog::error("User get failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception& e) {
        spdlog::error("User get failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace personal::v1
