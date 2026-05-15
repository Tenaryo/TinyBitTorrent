#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "peer/peer.hpp"

namespace {

TEST(PeerTest, MakeHandshake) {
    std::string info_hash(20, '\x01');
    std::string peer_id(20, '\x02');
    auto msg = peer::make_handshake(info_hash, peer_id);

    ASSERT_EQ(msg.size(), 68);
    EXPECT_EQ(msg[0], 19);
    EXPECT_EQ(msg.substr(1, 19), "BitTorrent protocol");
    for (int i = 20; i < 28; ++i) {
        EXPECT_EQ(msg[i], '\0');
    }
    EXPECT_EQ(msg.substr(28, 20), info_hash);
    EXPECT_EQ(msg.substr(48, 20), peer_id);
}

TEST(PeerTest, ParseHandshakePeerId) {
    std::string response(68, '\0');
    response[0] = 19;
    std::string protocol = "BitTorrent protocol";
    std::ranges::copy(protocol, response.begin() + 1);
    std::string expected_peer_id(20, '\x03');
    std::ranges::copy(expected_peer_id, response.begin() + 48);

    auto peer_id = peer::parse_handshake_peer_id(response);
    EXPECT_EQ(peer_id, expected_peer_id);
}

} // namespace
