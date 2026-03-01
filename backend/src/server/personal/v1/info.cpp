#include "info.hpp"

#include <boost/beast/http.hpp>
#include <pqxx/pqxx>

namespace http = boost::beast::http;

namespace personal::v1 {

auto handleInfo(const http::request<http::string_body>& req,
                ConnectionPool& pool) -> http::response<http::string_body> {

    try {
        auto h = pool.acquire();
        pqxx::work tx(h.conn());
        tx.exec("SELECT 1");
        tx.commit();
    } catch (const std::exception& e) {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(R"({"error":"db_error","details":")") + e.what() + R"("})";
        res.prepare_payload();
        return res;
    }

    const std::string jsonBody = R"({
  "id": 1,
  "username": "john_doe",
  "email": "john@example.com",
  "full_name": "John Doe",
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
