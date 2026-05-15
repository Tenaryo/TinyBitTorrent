#pragma once

#include <cstddef>
#include <random>
#include <string>

namespace util {

inline auto random_bytes(std::size_t n) -> std::string {
    thread_local std::random_device rdev;
    thread_local std::mt19937 gen(rdev());
    std::uniform_int_distribution<uint8_t> dist;
    std::string result(n, '\0');
    for (auto& byte : result) {
        byte = static_cast<char>(dist(gen));
    }
    return result;
}

} // namespace util
