#ifndef SERVER_PERSONAL_V1_INFO_HPP
#define SERVER_PERSONAL_V1_INFO_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace personal::v1 {

auto handleInfo(const http::request<http::string_body>& req,
                ConnectionPool& pool) -> http::response<http::string_body>;

} // namespace personal::v1

#endif // SERVER_PERSONAL_V1_INFO_HPP
