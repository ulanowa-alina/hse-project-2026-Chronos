#include "base64.hpp"

#include <vector>

namespace server::utils {

namespace {

constexpr const char* kBase64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

} // namespace

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

} // namespace server::utils
