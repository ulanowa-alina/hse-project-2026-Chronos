#pragma once

#include "server/session.hpp"

#include <functional>

namespace auth {
using AuthorizedHandler = std::function<Response(const Request&, int user_id)>;
RequestHandler with_auth(AuthorizedHandler handler);
} // namespace auth
