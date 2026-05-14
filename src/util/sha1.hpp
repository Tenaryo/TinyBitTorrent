#pragma once

#include <array>
#include <cstdint>
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

inline auto sha1_hex(std::string_view data) -> std::string {
    Sha1 hasher;
    hasher.update(data);
    auto digest = hasher.finalize();
    std::string hex(40, '\0');
    for (std::size_t idx = 0; idx < 20; ++idx) {
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
        hex[idx * 2] = kHex[digest[idx] >> 4];
        hex[idx * 2 + 1] = kHex[digest[idx] & 0xf];
    }
    return hex;
}

} // namespace util
