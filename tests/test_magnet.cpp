#include <gtest/gtest.h>

#include <string>

#include "magnet/magnet.hpp"
#include "output/output.hpp"
#include "util/sha1.hpp"

TEST(MagnetParse, ParseCompleteLink) {
    auto info = magnet::parse(
        "magnet:?xt=urn:btih:ad42ce8109f54c99613ce38f9b4d87e70f24a165"
        "&dn=magnet1.gif"
        "&tr=http%3A%2F%2Fbittorrent-test-tracker.codecrafters.io%2Fannounce");
    EXPECT_EQ(util::bytes_to_hex(info.info_hash_),
              "ad42ce8109f54c99613ce38f9b4d87e70f24a165");
    EXPECT_EQ(info.tracker_url_,
              "http://bittorrent-test-tracker.codecrafters.io/announce");
    EXPECT_EQ(info.display_name_, "magnet1.gif");
}

TEST(MagnetOutput, FormatMagnetInfo) {
    magnet::MagnetInfo info;
    info.info_hash_
        = util::hex_to_bytes("ad42ce8109f54c99613ce38f9b4d87e70f24a165");
    info.tracker_url_
        = "http://bittorrent-test-tracker.codecrafters.io/announce";
    auto formatted = output::format(info);
    EXPECT_EQ(
        formatted,
        "Tracker URL: http://bittorrent-test-tracker.codecrafters.io/announce\n"
        "Info Hash: ad42ce8109f54c99613ce38f9b4d87e70f24a165\n");
}
