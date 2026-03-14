#ifndef SERVER_BOARD_V1_CREATE_HPP
#define SERVER_BOARD_V1_CREATE_HPP

#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace board::v1 {

auto handleCreate(const http::request<http::string_body>& req) -> http::response<http::string_body>;

} // namespace board::v1

#endif // SERVER_BOARD_V1_CREATE_HPP
