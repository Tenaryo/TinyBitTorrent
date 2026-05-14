#include "util/url_encode.hpp"

#include <array>
#include <cstdint>

namespace util {

auto url_encode(std::string_view raw) -> std::string {
    static constexpr auto kIsUnreserved = [](char byte) -> bool {
        return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z')
               || (byte >= '0' && byte <= '9') || byte == '-' || byte == '_'
               || byte == '.' || byte == '~';
    };

    static constexpr std::array<char, 16> kHex{
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F',
    };

    std::string result;
    result.reserve(raw.size() * 3);
    for (char byte : raw) {
        if (kIsUnreserved(byte)) {
            result.push_back(byte);
        } else {
            auto val = static_cast<uint8_t>(byte);
            result.push_back('%');
            result.push_back(kHex[val >> 4]);
            result.push_back(kHex[val & 0xf]);
        }
    }
    return result;
}

} // namespace util
