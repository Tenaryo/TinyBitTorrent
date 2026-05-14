#include "torrent/metainfo.hpp"

#include <format>
#include <stdexcept>
#include <utility>

#include "bencode/encoder.hpp"
#include "util/sha1.hpp"

namespace torrent {

auto extract(const bencode::Dict& dict) -> Metainfo {
    bool found_announce = false;
    std::string announce;
    const bencode::Dict* info = nullptr;

    for (const auto& [key, val] : dict.items_) {
        if (key == "announce") {
            announce = std::get<bencode::String>(val);
            found_announce = true;
        } else if (key == "info") {
            info = &std::get<bencode::Dict>(val);
        }
    }

    if (!found_announce) {
        throw std::runtime_error("missing announce in torrent file");
    }
    if (info == nullptr) {
        throw std::runtime_error("missing info in torrent file");
    }

    bool found_length = false;
    int64_t length = 0;
    bool found_piece_length = false;
    int64_t piece_length = 0;
    bool found_pieces = false;
    std::vector<std::string> piece_hashes;

    for (const auto& [key, val] : info->items_) {
        if (key == "length") {
            length = std::get<bencode::Integer>(val);
            found_length = true;
        } else if (key == "piece length") {
            piece_length = std::get<bencode::Integer>(val);
            found_piece_length = true;
        } else if (key == "pieces") {
            const auto& raw = std::get<bencode::String>(val);
            if (raw.size() % 20 != 0) {
                throw std::runtime_error(
                    "invalid info.pieces: length not a multiple of 20");
            }
            for (std::size_t i = 0; i < raw.size(); i += 20) {
                std::string hex;
                hex.reserve(40);
                for (std::size_t j = 0; j < 20; ++j) {
                    std::format_to(std::back_inserter(hex),
                                   "{:02x}",
                                   static_cast<unsigned char>(raw[i + j]));
                }
                piece_hashes.push_back(std::move(hex));
            }
            found_pieces = true;
        }
    }

    if (!found_length) {
        throw std::runtime_error("missing info.length in torrent file");
    }
    if (!found_piece_length) {
        throw std::runtime_error("missing info.piece length in torrent file");
    }
    if (!found_pieces) {
        throw std::runtime_error("missing info.pieces in torrent file");
    }

    util::Sha1 hasher;
    hasher.update(bencode::encode(bencode::Value{*info}));
    auto digest = hasher.finalize();
    std::string info_hash(reinterpret_cast<const char*>(digest.data()),
                          digest.size());
    return {std::move(announce),
            length,
            std::move(info_hash),
            piece_length,
            std::move(piece_hashes)};
}

} // namespace torrent
