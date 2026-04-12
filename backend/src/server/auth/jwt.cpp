#include "jwt.hpp"

#include <cstdlib>
#include <ctime>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <optional>
#include <string>
#include <vector>

namespace auth {

namespace {

using nlohmann::json;

constexpr const char* kBase64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

auto get_jwt_secret() -> std::string {
    const char* env = std::getenv("JWT_SECRET");
    if (env != nullptr && *env != '\0') {
        return std::string(env);
    }
    return "dev-secret-change-me";
}

auto get_jwt_ttl_seconds() -> std::time_t {
    const char* env = std::getenv("JWT_TTL_SECONDS");
    if (env == nullptr || *env == '\0') {
        return 86400;
    }

    try {
        const long long ttl = std::stoll(env);
        if (ttl > 0) {
            return static_cast<std::time_t>(ttl);
        }
    } catch (...) {
    }

    return 86400;
}

auto base64_encode(const std::string& input) -> std::string {
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 3 <= input.size()) {
        const unsigned int b0 = static_cast<unsigned char>(input[i]);
        const unsigned int b1 = static_cast<unsigned char>(input[i + 1]);
        const unsigned int b2 = static_cast<unsigned char>(input[i + 2]);
        const unsigned int value = (b0 << 16U) | (b1 << 8U) | b2;

        output.push_back(kBase64Alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(value >> 12U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(value >> 6U) & 0x3FU]);
        output.push_back(kBase64Alphabet[value & 0x3FU]);
        i += 3;
    }

    const std::size_t rem = input.size() - i;
    if (rem == 1) {
        const unsigned int b0 = static_cast<unsigned char>(input[i]);
        const unsigned int value = (b0 << 16U);
        output.push_back(kBase64Alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(value >> 12U) & 0x3FU]);
        output.push_back('=');
        output.push_back('=');
    } else if (rem == 2) {
        const unsigned int b0 = static_cast<unsigned char>(input[i]);
        const unsigned int b1 = static_cast<unsigned char>(input[i + 1]);
        const unsigned int value = (b0 << 16U) | (b1 << 8U);
        output.push_back(kBase64Alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(value >> 12U) & 0x3FU]);
        output.push_back(kBase64Alphabet[(value >> 6U) & 0x3FU]);
        output.push_back('=');
    }

    return output;
}

auto base64_decode(const std::string& input) -> std::optional<std::string> {
    if (input.empty() || (input.size() % 4 != 0)) {
        return std::nullopt;
    }

    std::vector<int> lut(256, -1);
    for (int i = 0; i < 64; ++i) {
        lut[static_cast<unsigned char>(kBase64Alphabet[i])] = i;
    }

    std::string out;
    out.reserve((input.size() / 4) * 3);

    for (std::size_t i = 0; i < input.size(); i += 4) {
        const char c0 = input[i];
        const char c1 = input[i + 1];
        const char c2 = input[i + 2];
        const char c3 = input[i + 3];

        if (lut[static_cast<unsigned char>(c0)] < 0 || lut[static_cast<unsigned char>(c1)] < 0) {
            return std::nullopt;
        }

        const int v0 = lut[static_cast<unsigned char>(c0)];
        const int v1 = lut[static_cast<unsigned char>(c1)];

        int v2 = 0;
        int v3 = 0;
        const bool pad2 = (c2 == '=');
        const bool pad3 = (c3 == '=');

        if (!pad2) {
            v2 = lut[static_cast<unsigned char>(c2)];
            if (v2 < 0) {
                return std::nullopt;
            }
        }

        if (!pad3) {
            v3 = lut[static_cast<unsigned char>(c3)];
            if (v3 < 0) {
                return std::nullopt;
            }
        }

        const unsigned int value =
            (static_cast<unsigned int>(v0) << 18U) | (static_cast<unsigned int>(v1) << 12U) |
            (static_cast<unsigned int>(v2) << 6U) | static_cast<unsigned int>(v3);

        out.push_back(static_cast<char>((value >> 16U) & 0xFFU));
        if (!pad2) {
            out.push_back(static_cast<char>((value >> 8U) & 0xFFU));
        }
        if (!pad3) {
            out.push_back(static_cast<char>(value & 0xFFU));
        }
    }

    return out;
}

auto base64url_encode(const std::string& input) -> std::string {
    std::string encoded = base64_encode(input);
    for (char& c : encoded) {
        if (c == '+') {
            c = '-';
        } else if (c == '/') {
            c = '_';
        }
    }
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
}

auto base64url_decode(const std::string& input) -> std::optional<std::string> {
    std::string normalized = input;
    for (char& c : normalized) {
        if (c == '-') {
            c = '+';
        } else if (c == '_') {
            c = '/';
        }
    }
    while (normalized.size() % 4 != 0) {
        normalized.push_back('=');
    }
    return base64_decode(normalized);
}

auto hmac_sha256(const std::string& data, const std::string& secret) -> std::string {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest, &digest_len);

    return std::string(reinterpret_cast<const char*>(digest), digest_len);
}

auto timing_safe_equal(const std::string& lhs, const std::string& rhs) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    unsigned char diff = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<unsigned char>(lhs[i] ^ rhs[i]);
    }

    return diff == 0;
}

} // namespace

auto create_token(int user_id) -> std::string {
    const std::time_t now = std::time(nullptr);
    const std::time_t exp = now + get_jwt_ttl_seconds();

    const json header = {{"alg", "HS256"}, {"typ", "JWT"}};
    const json payload = {{"sub", user_id}, {"exp", exp}};

    const std::string encoded_header = base64url_encode(header.dump());
    const std::string encoded_payload = base64url_encode(payload.dump());
    const std::string signing_input = encoded_header + "." + encoded_payload;

    const std::string signature = hmac_sha256(signing_input, get_jwt_secret());
    const std::string encoded_signature = base64url_encode(signature);

    return signing_input + "." + encoded_signature;
}

auto parse_and_validate_token(const std::string& token, TokenPayload& payload,
                              TokenError& error) -> bool {
    const std::size_t first_dot = token.find('.');
    const std::size_t second_dot =
        token.find('.', first_dot == std::string::npos ? 0 : first_dot + 1);

    if (first_dot == std::string::npos || second_dot == std::string::npos ||
        second_dot + 1 >= token.size()) {
        error = TokenError::InvalidToken;
        return false;
    }

    const std::string encoded_header = token.substr(0, first_dot);
    const std::string encoded_payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string encoded_signature = token.substr(second_dot + 1);

    if (encoded_header.empty() || encoded_payload.empty() || encoded_signature.empty()) {
        error = TokenError::InvalidToken;
        return false;
    }

    const std::string signing_input = encoded_header + "." + encoded_payload;
    const std::string expected_signature =
        base64url_encode(hmac_sha256(signing_input, get_jwt_secret()));

    if (!timing_safe_equal(expected_signature, encoded_signature)) {
        error = TokenError::InvalidToken;
        return false;
    }

    const auto payload_raw = base64url_decode(encoded_payload);
    if (!payload_raw.has_value()) {
        error = TokenError::InvalidToken;
        return false;
    }

    try {
        const json payload_json = json::parse(*payload_raw);

        if (!payload_json.contains("sub") || !payload_json.contains("exp") ||
            !payload_json["sub"].is_number_integer() || !payload_json["exp"].is_number_integer()) {
            error = TokenError::InvalidToken;
            return false;
        }

        const int user_id = payload_json["sub"].get<int>();
        const std::time_t exp = payload_json["exp"].get<std::time_t>();

        if (user_id <= 0) {
            error = TokenError::InvalidToken;
            return false;
        }

        const std::time_t now = std::time(nullptr);
        if (now > exp) {
            error = TokenError::ExpiredToken;
            return false;
        }

        payload.user_id = user_id;
        return true;
    } catch (...) {
        error = TokenError::InvalidToken;
        return false;
    }
}

} // namespace auth
