#ifndef SERVER_PERSONAL_V1_AVATAR_UPLOAD_HPP
#define SERVER_PERSONAL_V1_AVATAR_UPLOAD_HPP

#include "db/connection_pool.hpp"

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace personal::v1 {

auto handleAvatarUpload(const http::request<http::string_body>& req, ConnectionPool& pool,
                        int user_id) -> http::response<http::string_body>;
auto handleAvatarDelete(const http::request<http::string_body>& req, ConnectionPool& pool,
                        int user_id) -> http::response<http::string_body>;

} // namespace personal::v1

#endif // SERVER_PERSONAL_V1_AVATAR_UPLOAD_HPP
