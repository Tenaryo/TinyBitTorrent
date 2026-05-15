#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace peer {

auto make_handshake(std::string_view info_hash, std::string_view our_peer_id)
    -> std::string;

auto parse_handshake_peer_id(std::string_view response) -> std::string;

auto handshake(std::string_view host,
               uint16_t port,
               std::string_view info_hash,
               std::string_view our_peer_id) -> std::string;

} // namespace peer
