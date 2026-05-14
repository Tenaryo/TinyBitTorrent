#include "torrent/metainfo.hpp"

#include <stdexcept>
#include <utility>

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

    for (const auto& [key, val] : info->items_) {
        if (key == "length") {
            length = std::get<bencode::Integer>(val);
            found_length = true;
            break;
        }
    }

    if (!found_length) {
        throw std::runtime_error("missing info.length in torrent file");
    }

    return {std::move(announce), length};
}

} // namespace torrent
