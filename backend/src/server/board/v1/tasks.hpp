#ifndef BOARD_TASKS_HPP
#define BOARD_TASKS_HPP

#include "../../../db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace board::v1 {

auto handleTasks(const http::request<http::string_body>& req, ConnectionPool& pool,
                 int user_id) -> http::response<http::string_body>;

} // namespace board::v1

#endif // BOARD_TASKS_HPP
