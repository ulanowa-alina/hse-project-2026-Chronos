#pragma once

#include <optional>
#include <string>

namespace server::utils {

auto base64_decode(const std::string& input) -> std::optional<std::string>;

} // namespace server::utils
