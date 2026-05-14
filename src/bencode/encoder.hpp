#pragma once

#include <string>

#include "bencode/value.hpp"

namespace bencode {

auto encode(const Value& value) -> std::string;

} // namespace bencode
