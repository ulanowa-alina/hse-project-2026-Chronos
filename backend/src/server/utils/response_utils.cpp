#include "response_utils.hpp"

namespace server::utils {

auto build_json_response(const http::request<http::string_body>& req, http::status status,
                         const nlohmann::json& body) -> http::response<http::string_body> {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

auto build_error_response(const http::request<http::string_body>& req, http::status status,
                          const std::string& code, const std::string& message,
                          const nlohmann::json& details) -> http::response<http::string_body> {
    nlohmann::json error = {
        {"code", code},
        {"message", message},
    };
    if (!details.is_null() && !details.empty()) {
        error["details"] = details;
    }
    return build_json_response(req, status, nlohmann::json{{"error", error}});
}

} // namespace server::utils
