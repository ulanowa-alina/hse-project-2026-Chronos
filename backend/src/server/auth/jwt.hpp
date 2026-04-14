#pragma once

#include <string>

namespace auth {

struct TokenPayload {
    int user_id{};
};

enum class TokenError { InvalidToken, ExpiredToken };

std::string create_token(int user_id);

bool parse_and_validate_token(const std::string& token, TokenPayload& payload, TokenError& error);

} // namespace auth
