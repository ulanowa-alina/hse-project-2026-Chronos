#ifndef SERVER_PERSONAL_V1_INFO_HPP
#define SERVER_PERSONAL_V1_INFO_HPP

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace personal::v1 {

http::response<http::string_body> handleInfo(const http::request<http::string_body>& req);

} // namespace personal::v1

#endif // SERVER_PERSONAL_V1_INFO_HPP
