#include "decoder.hpp"

#include <cctype>
#include <charconv>
#include <stdexcept>
#include <string>
#include <system_error>

namespace bencode {

namespace {

auto decode_string(std::string_view input) -> String {
    auto colon_pos = input.find(':');
    if (colon_pos == std::string_view::npos) {
        throw std::runtime_error("Invalid bencode string: missing ':'");
    }

    size_t length{};
    auto [ptr, ec]
        = std::from_chars(input.data(), input.data() + colon_pos, length);
    if (ec != std::errc{}) {
        throw std::runtime_error("Invalid bencode string: bad length prefix");
    }

    if (colon_pos + 1 + length > input.size()) {
        throw std::runtime_error(
            "Invalid bencode string: length exceeds input");
    }

    return String(input.substr(colon_pos + 1, length));
}

auto decode_integer(std::string_view input) -> Integer {
    if (input.front() != 'i') {
        throw std::runtime_error("Invalid bencode integer: expected 'i'");
    }

    auto end_pos = input.find('e', 1);
    if (end_pos == std::string_view::npos) {
        throw std::runtime_error(
            "Invalid bencode integer: missing terminator 'e'");
    }

    auto num_sv = input.substr(1, end_pos - 1);
    if (num_sv.empty()) {
        throw std::runtime_error("Invalid bencode integer: empty value");
    }

    if (num_sv.size() > 1 && num_sv.front() == '0') {
        throw std::runtime_error("Invalid bencode integer: leading zero");
    }
    if (num_sv.size() > 1 && num_sv.front() == '-' && num_sv[1] == '0') {
        throw std::runtime_error("Invalid bencode integer: -0 is illegal");
    }

    Integer result{};
    auto [ptr, ec]
        = std::from_chars(num_sv.data(), num_sv.data() + num_sv.size(), result);
    if (ec != std::errc{}) {
        throw std::runtime_error("Invalid bencode integer: not a valid number");
    }

    return result;
}

} // anonymous namespace

auto decode(std::string_view input) -> Value {
    if (input.empty()) {
        throw std::runtime_error("Empty bencode input");
    }

    auto first = static_cast<unsigned char>(input.front());

    if (std::isdigit(first) != 0) {
        return decode_string(input);
    }
    if (first == 'i') {
        return decode_integer(input);
    }

    throw std::runtime_error("Unhandled bencoded value: " + std::string(input));
}

} // namespace bencode
