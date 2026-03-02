#pragma once

#include <boost/beast/http.hpp>
#include <db/connection_pool.hpp>

namespace http = boost::beast::http;

namespace users {

auto handleCreate(const http::request<http::string_body>& req,
                  ConnectionPool& pool) -> http::response<http::string_body>;

} // namespace users
