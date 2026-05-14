#include "bencode/encoder.hpp"

#include <algorithm>
#include <format>
#include <variant>

namespace bencode {

namespace {

void append_to(std::string& out, const String& str) {
    std::format_to(std::back_inserter(out), "{}:", str.size());
    out.append(str);
}

void append_to(std::string& out, Integer val) {
    std::format_to(std::back_inserter(out), "i{}e", val);
}

void append_to(std::string& out, const List& list);
void append_to(std::string& out, Dict dict);

void append_to(std::string& out, const Value& value) {
    std::visit([&out](const auto& val) { append_to(out, val); }, value);
}

void append_to(std::string& out, const List& list) {
    out.push_back('l');
    for (const auto& elem : list.elements_) {
        append_to(out, elem);
    }
    out.push_back('e');
}

void append_to(std::string& out, Dict dict) {
    std::ranges::sort(
        dict.items_, {}, [](const auto& entry) { return entry.first; });
    out.push_back('d');
    for (const auto& [key, val] : dict.items_) {
        append_to(out, key);
        append_to(out, val);
    }
    out.push_back('e');
}

} // anonymous namespace

auto encode(const Value& value) -> std::string {
    std::string result;
    append_to(result, value);
    return result;
}

} // namespace bencode
