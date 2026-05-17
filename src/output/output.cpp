#include "output/output.hpp"

#include <format>
#include <type_traits>
#include <variant>

#include "lib/nlohmann/json.hpp"
#include "util/sha1.hpp"

namespace output {

namespace {

auto value_to_json(const bencode::Value& value) -> nlohmann::json {
    return std::visit(
        [](const auto& val) -> nlohmann::json {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bencode::List>) {
                auto arr = nlohmann::json::array();
                for (const auto& elem : val.elements_) {
                    arr.push_back(value_to_json(elem));
                }
                return arr;
            } else if constexpr (std::is_same_v<T, bencode::Dict>) {
                auto obj = nlohmann::json::object();
                for (const auto& [key, entry] : val.items_) {
                    obj[key] = value_to_json(entry);
                }
                return obj;
            } else {
                return nlohmann::json(val);
            }
        },
        value);
}

} // anonymous namespace

auto format(const bencode::Value& value) -> std::string {
    return value_to_json(value).dump();
}

auto format(const torrent::Metainfo& info) -> std::string {
    std::string result = std::format(
        "Tracker URL: {}\nLength: {}\nInfo Hash: {}\nPiece Length: {}\nPiece "
        "Hashes:\n",
        info.announce_,
        info.length_,
        util::bytes_to_hex(info.info_hash_),
        info.piece_length_);
    for (const auto& hash : info.piece_hashes_) {
        result += hash;
        result += '\n';
    }
    return result;
}

auto format(const magnet::MagnetInfo& info) -> std::string {
    return std::format("Tracker URL: {}\nInfo Hash: {}\n",
                       info.tracker_url_,
                       util::bytes_to_hex(info.info_hash_));
}

auto format(const peer::MagnetHandshakeResult& result) -> std::string {
    std::string output
        = std::format("Peer ID: {}\n", util::bytes_to_hex(result.peer_id_));
    if (result.metadata_ext_id_.has_value()) {
        output += std::format("Peer Metadata Extension ID: {}\n",
                              static_cast<int>(*result.metadata_ext_id_));
    }
    return output;
}

} // namespace output
