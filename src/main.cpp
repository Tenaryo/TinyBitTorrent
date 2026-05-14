#include <fstream>
#include <iostream>
#include <string>

#include "bencode/decoder.hpp"
#include "output/output.hpp"
#include "torrent/metainfo.hpp"
#include "tracker/tracker.hpp"

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
            auto peers = tracker::announce(metainfo, "-TB0001-0123456789AB");
            for (const auto& peer : peers) {
                std::cout << std::format("{}:{}\n", peer.ip_, peer.port_);
            }
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
