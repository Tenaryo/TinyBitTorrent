#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace bencode {

using String = std::string;
using Integer = int64_t;
using Value = std::variant<String, Integer>;

} // namespace bencode
