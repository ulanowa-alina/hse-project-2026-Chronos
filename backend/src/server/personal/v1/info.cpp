#include "info.hpp"

#include <boost/beast/http.hpp>
#include "repositories/user_repository.hpp"
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
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
    nlohmann::json out{
            {"id", user.id_},
            {"email", user.email_},
            {"name", user.name_},
            {"status", user.status_},
            {"created_at", to_iso8601(user.created_at_)}
    };

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

auto handleInfo(const http::request<http::string_body>& req,
                    ConnectionPool& pool) -> http::response<http::string_body> {
    try {
        UserRepository repo(pool);

        const auto user = repo.find_by_id(1);

        if (!user.has_value()) {
            return build_not_found_response(req);
        }

        return build_ok_response(req, *user);
    } catch (const std::exception& e) {
        return build_error_response(req, e);
    }
}


} // namespace personal::v1