#ifndef PASSWORD_HASHING_HPP
#define PASSWORD_HASHING_HPP

#include <string>

namespace security {

std::string hash_password(const std::string& password);
bool verify_password(const std::string& password, const std::string& password_hash);

} // namespace security

#endif
