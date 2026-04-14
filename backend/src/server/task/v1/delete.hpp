#ifndef TASK_V1_DELETE_HPP
#define TASK_V1_DELETE_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace task::v1 {

auto handleDelete(const http::request<http::string_body>& req,
                  ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body>;

} // namespace task::v1

#endif // TASK_V1_DELETE_HPP
