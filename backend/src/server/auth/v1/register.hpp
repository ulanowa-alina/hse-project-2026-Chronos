#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <boost/beast/http.hpp>
#include <db/connection_pool.hpp>

namespace http = boost::beast::http;

namespace auth::v1 {

auto handleRegister(const http::request<http::string_body>& req,
                    ConnectionPool& pool) -> http::response<http::string_body>;

} // namespace auth::v1

#endif
