#include "cmd/cmd.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "bencode/decoder.hpp"
#include "download/download.hpp"
#include "magnet/magnet.hpp"
#include "output/output.hpp"
#include "peer/peer.hpp"
#include "torrent/metainfo.hpp"
#include "tracker/tracker.hpp"
#include "util/net_util.hpp"
#include "util/random.hpp"
#include "util/sha1.hpp"

namespace cmd {

namespace {

struct Command {
    std::string_view name_;
    std::string_view usage_;
    int min_argc_;
    int (*handler_)(int argc, char** argv);
};

auto read_file(const char* path) -> std::string {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::string{"cannot open file: "} + path);
    }
    auto size = file.tellg();
    file.seekg(0);
    std::string buffer(static_cast<std::size_t>(size), '\0');
    file.read(buffer.data(), size);
    return buffer;
}

auto parse_torrent(const char* path) -> torrent::Metainfo {
    auto raw = read_file(path);
    auto value = bencode::decode(raw);
    const auto& dict = std::get<bencode::Dict>(value);
    return torrent::extract(dict);
}

auto handle_decode(const char* arg) -> int {
    auto decoded = bencode::decode(arg);
    std::cout << output::format(decoded) << '\n';
    return 0;
}

auto handle_info(const char* torrent_path) -> int {
    auto metainfo = parse_torrent(torrent_path);
    std::cout << output::format(metainfo);
    return 0;
}

auto handle_peers(const char* torrent_path) -> int {
    auto metainfo = parse_torrent(torrent_path);
    auto peers = tracker::announce(metainfo, util::random_bytes(20));
    for (const auto& peer : peers) {
        std::cout << std::format("{}:{}\n", peer.ip_, peer.port_);
    }
    return 0;
}

auto handle_download_piece(const char* output_path,
                           const char* torrent_path,
                           const char* piece_index_str) -> int {
    auto metainfo = parse_torrent(torrent_path);
    auto piece_index = std::stoi(piece_index_str);
    auto peer_id = util::random_bytes(20);
    auto peers = tracker::announce(metainfo, peer_id);
    if (peers.empty()) {
        throw std::runtime_error("no peers available");
    }
    auto data = peer::download_piece(
        metainfo, peers[0].ip_, peers[0].port_, peer_id, piece_index);
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error(std::string{"cannot write to "} + output_path);
    }
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return 0;
}

auto handle_download(const char* output_path, const char* torrent_path) -> int {
    auto metainfo = parse_torrent(torrent_path);
    auto peers = tracker::announce(metainfo, util::random_bytes(20));
    download::download_file(metainfo, peers, output_path);
    return 0;
}

auto handle_handshake(const char* torrent_path, const char* host_port) -> int {
    auto metainfo = parse_torrent(torrent_path);
    auto [host, port] = util::parse_host_port(host_port);
    auto peer_id = util::random_bytes(20);
    auto result = peer::handshake(host, port, metainfo.info_hash_, peer_id);
    std::cout << "Peer ID: " << util::bytes_to_hex(result) << '\n';
    return 0;
}

auto handle_magnet_parse(const char* magnet_link) -> int {
    auto info = magnet::parse(magnet_link);
    std::cout << output::format(info);
    return 0;
}

auto handle_magnet_handshake(const char* magnet_link) -> int {
    auto info = magnet::parse(magnet_link);
    auto peer_id = util::random_bytes(20);
    auto peers = tracker::announce(info.info_hash_, info.tracker_url_, peer_id);
    if (peers.empty()) {
        throw std::runtime_error("no peers available");
    }
    auto result = peer::magnet_handshake(
        peers[0].ip_, peers[0].port_, info.info_hash_, peer_id);
    std::cout << output::format(result);
    return 0;
}

auto cmd_decode(int /*argc*/, char** argv) -> int {
    return handle_decode(argv[2]);
}

auto cmd_info(int /*argc*/, char** argv) -> int {
    return handle_info(argv[2]);
}

auto cmd_peers(int /*argc*/, char** argv) -> int {
    return handle_peers(argv[2]);
}

auto cmd_download_piece(int /*argc*/, char** argv) -> int {
    return handle_download_piece(argv[3], argv[4], argv[5]);
}

auto cmd_download(int /*argc*/, char** argv) -> int {
    return handle_download(argv[3], argv[4]);
}

auto cmd_handshake(int /*argc*/, char** argv) -> int {
    return handle_handshake(argv[2], argv[3]);
}

auto cmd_magnet_parse(int /*argc*/, char** argv) -> int {
    return handle_magnet_parse(argv[2]);
}

auto cmd_magnet_handshake(int /*argc*/, char** argv) -> int {
    return handle_magnet_handshake(argv[2]);
}

auto handle_magnet_info(const char* magnet_link) -> int {
    auto info = magnet::parse(magnet_link);
    auto peer_id = util::random_bytes(20);
    auto peers = tracker::announce(info.info_hash_, info.tracker_url_, peer_id);
    if (peers.empty()) {
        throw std::runtime_error("no peers available");
    }
    auto metainfo = peer::magnet_info(
        peers[0].ip_, peers[0].port_, info.info_hash_, peer_id);
    metainfo.announce_ = info.tracker_url_;
    std::cout << output::format(metainfo);
    return 0;
}

auto cmd_magnet_info(int /*argc*/, char** argv) -> int {
    return handle_magnet_info(argv[2]);
}

constexpr std::array kCommands = {
    Command{"decode", "decode <encoded_value>", 3, cmd_decode},
    Command{"info", "info <torrent_file>", 3, cmd_info},
    Command{"peers", "peers <torrent_file>", 3, cmd_peers},
    Command{"download_piece",
            "download_piece -o <output> <torrent_file> <piece_index>",
            6,
            cmd_download_piece},
    Command{"download", "download -o <output> <torrent_file>", 5, cmd_download},
    Command{"handshake",
            "handshake <torrent_file> <peer_ip:peer_port>",
            4,
            cmd_handshake},
    Command{"magnet_parse", "magnet_parse <magnet_link>", 3, cmd_magnet_parse},
    Command{"magnet_handshake",
            "magnet_handshake <magnet_link>",
            3,
            cmd_magnet_handshake},
    Command{"magnet_info", "magnet_info <magnet_link>", 3, cmd_magnet_info},
};

} // anonymous namespace

auto dispatch(std::string_view command, int argc, char** argv) -> int {
    for (const auto& cmd : kCommands) {
        if (command == cmd.name_) {
            if (argc < cmd.min_argc_) {
                std::cerr << "Usage: " << argv[0] << ' ' << cmd.usage_ << '\n';
                return 1;
            }
            return cmd.handler_(argc, argv);
        }
    }
    std::cerr << "unknown command: " << command << '\n';
    return 1;
}

} // namespace cmd
