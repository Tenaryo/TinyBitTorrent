#pragma once

#include <cstdint>
#include <string>

namespace util {

inline void write_int32_be(std::string& buf, int32_t val) {
    buf.push_back(static_cast<char>((val >> 24) & 0xFF));
    buf.push_back(static_cast<char>((val >> 16) & 0xFF));
    buf.push_back(static_cast<char>((val >> 8) & 0xFF));
    buf.push_back(static_cast<char>(val & 0xFF));
}

inline auto read_int32_be(const char*& data) noexcept -> int32_t {
    auto byte0 = static_cast<uint8_t>(data[0]);
    auto byte1 = static_cast<uint8_t>(data[1]);
    auto byte2 = static_cast<uint8_t>(data[2]);
    auto byte3 = static_cast<uint8_t>(data[3]);
    data += 4;
    return static_cast<int32_t>((byte0 << 24) | (byte1 << 16) | (byte2 << 8)
                                | byte3);
}

} // namespace util
