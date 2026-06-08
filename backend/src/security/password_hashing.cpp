#include "password_hashing.hpp"

#include <argon2.h>
#include <array>
#include <cstdint>
#include <openssl/rand.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace security {

namespace {

constexpr std::uint32_t TIME_COST = 3;
constexpr std::uint32_t MEMORY_COST = 1 << 16;
constexpr std::uint32_t PARALLELISM = 1;
constexpr std::uint32_t HASH_LENGTH = 32;
constexpr std::size_t SALT_LENGTH = 16;

std::array<unsigned char, SALT_LENGTH> generate_salt() {
    std::array<unsigned char, SALT_LENGTH> salt{};

    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("Failed to generate password salt");
    }

    return salt;
}

} // namespace

std::string hash_password(const std::string& password) {
    const auto salt = generate_salt();
    const auto encoded_length =
        argon2_encodedlen(TIME_COST, MEMORY_COST, PARALLELISM, SALT_LENGTH, HASH_LENGTH, Argon2_id);
    std::vector<char> encoded(encoded_length);

    const int result = argon2id_hash_encoded(TIME_COST, MEMORY_COST, PARALLELISM, password.data(),
                                             password.size(), salt.data(), salt.size(), HASH_LENGTH,
                                             encoded.data(), encoded.size());

    if (result != ARGON2_OK) {
        throw std::runtime_error(argon2_error_message(result));
    }

    return std::string(encoded.data());
}

bool verify_password(const std::string& password, const std::string& password_hash) {
    const int result = argon2id_verify(password_hash.c_str(), password.data(), password.size());

    return result == ARGON2_OK;
}

} // namespace security
