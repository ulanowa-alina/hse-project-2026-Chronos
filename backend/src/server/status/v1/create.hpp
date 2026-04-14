#ifndef STATUS_V1_CREATE_HPP
#define STATUS_V1_CREATE_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace status::v1 {

auto handleCreate(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body>;

} // namespace status::v1

#endif
