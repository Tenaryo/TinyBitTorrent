#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bencode/value.hpp"

namespace torrent {

struct Metainfo {
    std::string announce_;
    int64_t length_{};
    std::string info_hash_; // 20-byte raw SHA1
    int64_t piece_length_{};
    std::vector<std::string> piece_hashes_;
};

auto extract(const bencode::Dict& dict) -> Metainfo;

auto from_info_dict(const bencode::Dict& info_dict) -> Metainfo;

} // namespace torrent
