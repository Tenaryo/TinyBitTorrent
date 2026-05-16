#include "tracker/tracker.hpp"

#include <format>
#include <stdexcept>

#include "bencode/decoder.hpp"
#include "http/http_client.hpp"
#include "util/url_encode.hpp"

namespace tracker {

namespace {

auto build_announce_url(std::string_view tracker_url,
                        std::string_view info_hash,
                        std::string_view peer_id,
                        int64_t left) -> std::string {
    return std::format(
        "{}?info_hash={}&peer_id={}&port=6881&uploaded=0&downloaded=0&left={}"
        "&compact=1",
        tracker_url,
        util::url_encode(info_hash),
        util::url_encode(peer_id),
        left);
}

} // anonymous namespace

auto announce(const torrent::Metainfo& info, std::string_view peer_id)
    -> std::vector<Peer> {
    auto url = build_announce_url(
        info.announce_, info.info_hash_, peer_id, info.length_);

    auto response = http::get(url);
    auto parsed = bencode::decode(response);

    const auto* dict = std::get_if<bencode::Dict>(&parsed);
    if (dict == nullptr) {
        throw std::runtime_error(
            "tracker response is not a bencoded dictionary");
    }

    const bencode::String* peers_raw = nullptr;
    for (const auto& [key, val] : dict->items_) {
        if (key == "peers") {
            peers_raw = std::get_if<bencode::String>(&val);
        }
    }

    if (peers_raw == nullptr) {
        throw std::runtime_error("tracker response missing 'peers' key");
    }

    if (peers_raw->size() % 6 != 0) {
        throw std::runtime_error(
            "invalid peers string: length not a multiple of 6");
    }

    std::vector<Peer> result;
    result.reserve(peers_raw->size() / 6);
    for (std::size_t i = 0; i < peers_raw->size(); i += 6) {
        Peer peer;
        peer.ip_ = std::format("{}.{}.{}.{}",
                               static_cast<uint8_t>((*peers_raw)[i]),
                               static_cast<uint8_t>((*peers_raw)[i + 1]),
                               static_cast<uint8_t>((*peers_raw)[i + 2]),
                               static_cast<uint8_t>((*peers_raw)[i + 3]));
        auto port = static_cast<uint16_t>(
            (static_cast<uint32_t>(static_cast<uint8_t>((*peers_raw)[i + 4]))
             << 8)
            | static_cast<uint32_t>(static_cast<uint8_t>((*peers_raw)[i + 5])));
        peer.port_ = port;
        result.push_back(std::move(peer));
    }
    return result;
}

auto announce(std::string_view info_hash,
              std::string_view tracker_url,
              std::string_view peer_id) -> std::vector<Peer> {
    auto url = build_announce_url(tracker_url, info_hash, peer_id, 0);

    auto response = http::get(url);
    auto parsed = bencode::decode(response);

    const auto* dict = std::get_if<bencode::Dict>(&parsed);
    if (dict == nullptr) {
        throw std::runtime_error(
            "tracker response is not a bencoded dictionary");
    }

    const bencode::String* peers_raw = nullptr;
    for (const auto& [key, val] : dict->items_) {
        if (key == "peers") {
            peers_raw = std::get_if<bencode::String>(&val);
        }
    }

    if (peers_raw == nullptr) {
        throw std::runtime_error("tracker response missing 'peers' key");
    }

    if (peers_raw->size() % 6 != 0) {
        throw std::runtime_error(
            "invalid peers string: length not a multiple of 6");
    }

    std::vector<Peer> result;
    result.reserve(peers_raw->size() / 6);
    for (std::size_t i = 0; i < peers_raw->size(); i += 6) {
        Peer peer;
        peer.ip_ = std::format("{}.{}.{}.{}",
                               static_cast<uint8_t>((*peers_raw)[i]),
                               static_cast<uint8_t>((*peers_raw)[i + 1]),
                               static_cast<uint8_t>((*peers_raw)[i + 2]),
                               static_cast<uint8_t>((*peers_raw)[i + 3]));
        auto port = static_cast<uint16_t>(
            (static_cast<uint32_t>(static_cast<uint8_t>((*peers_raw)[i + 4]))
             << 8)
            | static_cast<uint32_t>(static_cast<uint8_t>((*peers_raw)[i + 5])));
        peer.port_ = port;
        result.push_back(std::move(peer));
    }
    return result;
}

} // namespace tracker
