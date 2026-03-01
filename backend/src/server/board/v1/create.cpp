#include "create.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

namespace board::v1 {

auto handleCreate(const http::request<http::string_body>& req)
    -> http::response<http::string_body> {
    const std::string jsonBody = R"({
  "id": 1,
  "name": "My Board",
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

} // namespace board::v1
