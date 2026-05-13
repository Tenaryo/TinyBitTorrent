#pragma once

#include <string_view>

#include "value.hpp"

namespace bencode {

auto decode(std::string_view input) -> Value;

} // namespace bencode
