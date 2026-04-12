#pragma once

#include "server/session.hpp"

#include <functional>

namespace auth {
using AuthorizedHandler = std::function<Response(const Request&, int user_id)>;
auto with_auth(AuthorizedHandler handler) -> RequestHandler;
} // namespace auth
