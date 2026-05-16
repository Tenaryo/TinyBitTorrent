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

auto url_decode(std::string_view encoded) -> std::string {
    static constexpr auto kHexValue = []() {
        std::array<int, 256> table{};
        table.fill(-1);
        for (int i = 0; i < 10; ++i) {
            table[static_cast<std::size_t>('0') + static_cast<std::size_t>(i)]
                = i;
        }
        for (int i = 0; i < 6; ++i) {
            table[static_cast<std::size_t>('A') + static_cast<std::size_t>(i)]
                = 10 + i;
            table[static_cast<std::size_t>('a') + static_cast<std::size_t>(i)]
                = 10 + i;
        }
        return table;
    }();

    std::string result;
    result.reserve(encoded.size());
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int high = kHexValue[static_cast<uint8_t>(encoded[i + 1])];
            int low = kHexValue[static_cast<uint8_t>(encoded[i + 2])];
            if (high >= 0 && low >= 0) {
                result.push_back(static_cast<char>((high << 4) | low));
                i += 2;
                continue;
            }
        }
        result.push_back(encoded[i]);
    }
    return result;
}

} // namespace util
