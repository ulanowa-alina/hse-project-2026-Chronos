#include "info.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace personal::v1 {

http::response<http::string_body> handleInfo(const http::request<http::string_body>& req) {
    const std::string jsonBody = R"({
  "id": 1,
  "username": "john",
  "email": "john@example.com",
  "full_name": "John",
  "created_at": "2024-01-15T10:00:00Z"
})";

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = jsonBody;
    res.prepare_payload();
    return res;
}

} // namespace personal::v1
