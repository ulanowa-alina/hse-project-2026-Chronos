#include "with_auth.hpp"

#include "server/auth/jwt.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace auth {

namespace {

auto build_auth_error(const Request& req, http::status status, const std::string& code,
                      const std::string& message) -> Response {
    Response res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = nlohmann::json{{"error", {{"code", code}, {"message", message}}}}.dump();
    res.prepare_payload();
    return res;
}

} // namespace

auto with_auth(AuthorizedHandler handler) -> RequestHandler {
    return [handler = std::move(handler)](const Request& req) -> Response {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return build_auth_error(req, http::status::unauthorized, "UNAUTHORIZED",
                                    "User is not authorized");
        }

        const std::string auth_value = std::string(auth_header);
        const std::string prefix = "Bearer ";
        if (auth_value.rfind(prefix, 0) != 0) {
            return build_auth_error(req, http::status::unauthorized, "INVALID_TOKEN",
                                    "Invalid token");
        }

        TokenPayload payload;
        TokenError token_error = TokenError::InvalidToken;
        if (!parse_and_validate_token(auth_value.substr(prefix.size()), payload, token_error)) {
            if (token_error == TokenError::ExpiredToken) {
                return build_auth_error(req, http::status::unauthorized, "INVALID_TOKEN",
                                        "Token is expired");
            }

            return build_auth_error(req, http::status::unauthorized, "INVALID_TOKEN",
                                    "Invalid token");
        }

        return handler(req, payload.user_id);
    };
}

} // namespace auth
