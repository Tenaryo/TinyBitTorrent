#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "torrent/metainfo.hpp"

namespace peer {

struct MagnetHandshakeResult {
    std::string peer_id_;
    std::optional<uint8_t> metadata_ext_id_;
};

auto make_handshake(std::string_view info_hash, std::string_view our_peer_id)
    -> std::string;

auto parse_handshake_peer_id(std::string_view response) -> std::string;

auto handshake(std::string_view host,
               uint16_t port,
               std::string_view info_hash,
               std::string_view our_peer_id) -> std::string;

auto magnet_handshake(std::string_view host,
                      uint16_t port,
                      std::string_view info_hash,
                      std::string_view our_peer_id,
                      uint8_t ext_id = 1) -> MagnetHandshakeResult;

auto parse_ext_handshake_response(std::string_view bencode_payload) -> uint8_t;

auto handshake_has_extensions(std::string_view response) -> bool;

auto ext_handshake(int sock_fd,
                   std::string_view info_hash,
                   std::string_view our_peer_id,
                   uint8_t ext_id) -> MagnetHandshakeResult;

auto build_metadata_request(uint8_t peer_ext_id, int piece_index)
    -> std::string;

auto send_metadata_request(int sock_fd, uint8_t peer_ext_id, int piece_index)
    -> void;

auto magnet_info(std::string_view host,
                 uint16_t port,
                 std::string_view info_hash,
                 std::string_view our_peer_id,
                 uint8_t ext_id = 1) -> torrent::Metainfo;

auto parse_metadata_data(std::string_view payload) -> std::string;

auto download_piece(const torrent::Metainfo& info,
                    std::string_view peer_ip,
                    uint16_t peer_port,
                    std::string_view our_peer_id,
                    int piece_index,
                    int pipeline_depth = 5) -> std::string;

} // namespace peer
