#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "torrent/metainfo.hpp"

namespace tracker {

struct Peer {
    std::string ip_;
    uint16_t port_;
};

auto announce(const torrent::Metainfo& info, std::string_view peer_id)
    -> std::vector<Peer>;

auto announce(std::string_view info_hash,
              std::string_view tracker_url,
              std::string_view peer_id) -> std::vector<Peer>;

} // namespace tracker
