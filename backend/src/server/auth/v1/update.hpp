#pragma once

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace auth::v1 {

auto handleUpdate(const http::request<http::string_body>& req,
                  ConnectionPool& pool) -> http::response<http::string_body>;

} // namespace auth::v1
