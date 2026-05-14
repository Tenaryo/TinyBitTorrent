#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "bencode/decoder.hpp"
#include "output/output.hpp"
#include "torrent/metainfo.hpp"

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

TEST(EndToEnd, SampleTorrentInfo) {
    auto raw = read_file(SAMPLE_TORRENT);
    auto value = bencode::decode(raw);
    const auto& dict = std::get<bencode::Dict>(value);
    auto info = torrent::extract(dict);
    auto output = output::format(info);

    EXPECT_EQ(output,
              "Tracker URL: "
              "http://bittorrent-test-tracker.codecrafters.io/announce\n"
              "Length: 92063\n");
}
