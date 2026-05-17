#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace bencode {

using String = std::string;
using Integer = int64_t;

struct List;
struct Dict;

using Value = std::variant<String, Integer, List, Dict>;

struct List {
    std::vector<Value> elements_;
};

struct Dict {
    using Entry = std::pair<String, Value>;
    std::vector<Entry> items_;
};

[[nodiscard]] inline auto find(const Dict& dict, std::string_view key) noexcept
    -> const Value* {
    for (const auto& [k, v] : dict.items_) {
        if (k == key) {
            return &v;
        }
    }
    return nullptr;
}

} // namespace bencode
