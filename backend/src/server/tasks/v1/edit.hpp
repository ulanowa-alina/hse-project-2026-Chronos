#ifndef TASKS_V1_EDIT_HPP
#define TASKS_V1_EDIT_HPP

#include "../../../db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace tasks::v1 {

auto handleEdit(const http::request<http::string_body>& req,
                ConnectionPool& pool) -> http::response<http::string_body>;

} // namespace tasks::v1

#endif // TASKS_V1_EDIT_HPP
