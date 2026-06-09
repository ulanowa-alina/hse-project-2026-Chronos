#ifndef POMODORO_V1_GET_USER_SESSIONS_HPP
#define POMODORO_V1_GET_USER_SESSIONS_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace pomodoro::v1 {

auto handleGetUserSessions(const http::request<http::string_body>& req, ConnectionPool& pool,
                           int user_id) -> http::response<http::string_body>;

} // namespace pomodoro::v1

#endif // POMODORO_V1_GET_USER_SESSIONS_HPP
