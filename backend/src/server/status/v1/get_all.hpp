#ifndef STATUS_V1_GET_ALL_HPP
#define STATUS_V1_GET_ALL_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace status::v1 {

auto handleGetAll(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body>;

} // namespace status::v1

#endif // STATUS_V1_GET_ALL_HPP
