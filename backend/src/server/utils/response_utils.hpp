#ifndef SERVER_UTILS_RESPONSE_UTILS_HPP
#define SERVER_UTILS_RESPONSE_UTILS_HPP

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace http = boost::beast::http;

namespace server::utils {

auto build_json_response(const http::request<http::string_body>& req, http::status status,
                         const nlohmann::json& body) -> http::response<http::string_body>;

auto build_error_response(const http::request<http::string_body>& req, http::status status,
                          const std::string& code, const std::string& message,
                          const nlohmann::json& details = nlohmann::json())
    -> http::response<http::string_body>;

} // namespace server::utils

#endif // SERVER_UTILS_RESPONSE_UTILS_HPP
