#pragma once

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace personal::v1 {

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body>;

} // namespace personal::v1
