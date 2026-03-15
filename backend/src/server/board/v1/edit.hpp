#ifndef BOARD_V1_EDIT_HPP
#define BOARD_V1_EDIT_HPP

#include "../../repositories/board_repository.hpp"
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace board::v1 {

http::response<http::string_body> handleEdit(const http::request<http::string_body>& req,
                                             BoardRepository& repo);

} // namespace board::v1

#endif // BOARD_V1_EDIT_HPP
