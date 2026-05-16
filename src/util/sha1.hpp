#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace util {

class Sha1 {
  public:
    Sha1();

    void update(std::string_view data);
    auto finalize() -> std::array<uint8_t, 20>;

  private:
    void process_block();

    std::array<uint32_t, 5> h_;
    std::array<uint8_t, 64> block_;
    std::size_t block_len_;
    uint64_t total_len_;
};

inline auto bytes_to_hex(std::string_view bytes) -> std::string {
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
        'a',
        'b',
        'c',
        'd',
        'e',
        'f',
    };
    std::string hex(bytes.size() * 2, '\0');
    for (std::size_t idx = 0; idx < bytes.size(); ++idx) {
        auto byte = static_cast<uint8_t>(bytes[idx]);
        hex[idx * 2] = kHex[byte >> 4];
        hex[idx * 2 + 1] = kHex[byte & 0xf];
    }
    return hex;
}

inline auto hex_to_bytes(std::string_view hex) -> std::string {
    if (hex.size() != 40) {
        throw std::runtime_error("info hash must be 40 hex characters");
    }
    static constexpr auto kHexValue = []() {
        std::array<int, 256> table{};
        table.fill(-1);
        for (int i = 0; i < 10; ++i) {
            table[static_cast<std::size_t>('0') + static_cast<std::size_t>(i)]
                = i;
        }
        for (int i = 0; i < 6; ++i) {
            table[static_cast<std::size_t>('a') + static_cast<std::size_t>(i)]
                = 10 + i;
            table[static_cast<std::size_t>('A') + static_cast<std::size_t>(i)]
                = 10 + i;
        }
        return table;
    }();
    std::string bytes(20, '\0');
    for (std::size_t i = 0; i < 20; ++i) {
        int high = kHexValue[static_cast<uint8_t>(hex[i * 2])];
        int low = kHexValue[static_cast<uint8_t>(hex[i * 2 + 1])];
        if (high < 0 || low < 0) {
            throw std::runtime_error("invalid hex character in info hash");
        }
        bytes[i] = static_cast<char>((high << 4) | low);
    }
    return bytes;
}

inline auto sha1_hex(std::string_view data) -> std::string {
    Sha1 hasher;
    hasher.update(data);
    auto digest = hasher.finalize();
    return bytes_to_hex(std::string_view{
        reinterpret_cast<const char*>(digest.data()), digest.size()});
}

} // namespace util
