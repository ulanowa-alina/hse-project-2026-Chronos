#ifndef BOARD_V1_EDIT_HPP
#define BOARD_V1_EDIT_HPP

#include "../../../db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace board::v1 {

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body>;

} // namespace board::v1

#endif // BOARD_V1_EDIT_HPP
