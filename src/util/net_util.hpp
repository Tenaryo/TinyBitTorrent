#pragma once

#include <charconv>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace util {

inline auto parse_host_port(std::string_view addr)
    -> std::pair<std::string, uint16_t> {
    auto colon = addr.find(':');
    if (colon == std::string_view::npos) {
        throw std::runtime_error("invalid address: missing port");
    }
    auto host = std::string{addr.substr(0, colon)};
    auto port_sv = addr.substr(colon + 1);
    int port = 0;
    auto [_, ec] = std::from_chars(
        port_sv.data(), port_sv.data() + port_sv.size(), port);
    if (ec != std::errc{} || port < 1 || port > 65535) {
        throw std::runtime_error(std::format("invalid port: {}", port_sv));
    }
    return {std::move(host), static_cast<uint16_t>(port)};
}

} // namespace util
