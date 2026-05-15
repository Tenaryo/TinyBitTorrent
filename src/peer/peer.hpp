#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "torrent/metainfo.hpp"

namespace peer {

auto make_handshake(std::string_view info_hash, std::string_view our_peer_id)
    -> std::string;

auto parse_handshake_peer_id(std::string_view response) -> std::string;

auto handshake(std::string_view host,
               uint16_t port,
               std::string_view info_hash,
               std::string_view our_peer_id) -> std::string;

auto download_piece(const torrent::Metainfo& info,
                    std::string_view peer_ip,
                    uint16_t peer_port,
                    std::string_view our_peer_id,
                    int piece_index,
                    int pipeline_depth = 5) -> std::string;

} // namespace peer
