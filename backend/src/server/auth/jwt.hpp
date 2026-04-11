#pragma once

#include <string>

namespace auth {

struct TokenPayload {
    int user_id{};
};

enum class TokenError { InvalidToken, ExpiredToken };

auto create_token(int user_id) -> std::string;

auto parse_and_validate_token(const std::string& token, TokenPayload& payload,
                              TokenError& error) -> bool;

} // namespace auth
