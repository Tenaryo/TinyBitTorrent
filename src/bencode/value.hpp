#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace bencode {

using String = std::string;
using Integer = int64_t;

struct List;

using Value = std::variant<String, Integer, List>;

struct List {
    std::vector<Value> elements_;
};

} // namespace bencode
