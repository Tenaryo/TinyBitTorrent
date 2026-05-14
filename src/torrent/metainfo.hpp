#pragma once

#include <cstdint>
#include <string>

#include "bencode/value.hpp"

namespace torrent {

struct Metainfo {
    std::string announce_;
    int64_t length_;
    std::string info_hash_;
};

auto extract(const bencode::Dict& dict) -> Metainfo;

} // namespace torrent
