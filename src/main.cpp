#include <fstream>
#include <iostream>
#include <string>

#include "bencode/decoder.hpp"
#include "output/output.hpp"
#include "peer/peer.hpp"
#include "torrent/metainfo.hpp"
#include "tracker/tracker.hpp"
#include "util/net_util.hpp"
#include "util/random.hpp"
#include "util/sha1.hpp"

namespace {

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

} // anonymous namespace

auto main(int argc, char* argv[]) -> int {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <command> [args]\n";
            return 1;
        }

        std::string command = argv[1];

        if (command == "decode") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0]
                          << " decode <encoded_value>\n";
                return 1;
            }
            auto decoded = bencode::decode(argv[2]);
            std::cout << output::format(decoded) << '\n';
        } else if (command == "info") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0] << " info <torrent_file>\n";
                return 1;
            }
            auto raw = read_file(argv[2]);
            auto value = bencode::decode(raw);
            const auto& dict = std::get<bencode::Dict>(value);
            auto metainfo = torrent::extract(dict);
            std::cout << output::format(metainfo);
        } else if (command == "peers") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0] << " peers <torrent_file>\n";
                return 1;
            }
            auto raw = read_file(argv[2]);
            auto value = bencode::decode(raw);
            const auto& dict = std::get<bencode::Dict>(value);
            auto metainfo = torrent::extract(dict);
            auto peers = tracker::announce(metainfo, util::random_bytes(20));
            for (const auto& peer : peers) {
                std::cout << std::format("{}:{}\n", peer.ip_, peer.port_);
            }
        } else if (command == "handshake") {
            if (argc < 4) {
                std::cerr << "Usage: " << argv[0]
                          << " handshake <torrent_file> <peer_ip:peer_port>\n";
                return 1;
            }
            auto raw = read_file(argv[2]);
            auto value = bencode::decode(raw);
            const auto& dict = std::get<bencode::Dict>(value);
            auto metainfo = torrent::extract(dict);
            auto [host, port] = util::parse_host_port(argv[3]);
            auto peer_id = util::random_bytes(20);
            auto result
                = peer::handshake(host, port, metainfo.info_hash_, peer_id);
            std::cout << "Peer ID: " << util::bytes_to_hex(result) << '\n';
        } else {
            std::cerr << "unknown command: " << command << '\n';
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
