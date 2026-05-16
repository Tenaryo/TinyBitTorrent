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
    EXPECT_EQ(msg[25], '\x10');
    for (int i = 20; i < 28; ++i) {
        if (i == 25) {
            continue;
        }
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

TEST(PeerTest, ParseHandshakePeerIdWithExtensionBits) {
    std::string response(68, '\0');
    response[0] = 19;
    std::string protocol = "BitTorrent protocol";
    std::ranges::copy(protocol, response.begin() + 1);
    response[25] = '\x10';
    std::string expected_peer_id(20, '\x03');
    std::ranges::copy(expected_peer_id, response.begin() + 48);

    auto peer_id = peer::parse_handshake_peer_id(response);
    EXPECT_EQ(peer_id, expected_peer_id);
}

TEST(PeerTest, HandshakeHasExtensionsWhenBitSet) {
    std::string response(68, '\0');
    response[0] = 19;
    std::string protocol = "BitTorrent protocol";
    std::ranges::copy(protocol, response.begin() + 1);
    response[25] = '\x10';
    EXPECT_TRUE(peer::handshake_has_extensions(response));
}

TEST(PeerTest, HandshakeHasExtensionsWhenBitNotSet) {
    std::string response(68, '\0');
    response[0] = 19;
    std::string protocol = "BitTorrent protocol";
    std::ranges::copy(protocol, response.begin() + 1);
    response[25] = '\x00';
    EXPECT_FALSE(peer::handshake_has_extensions(response));
}

TEST(PeerTest, HandshakeHasExtensionsTooShort) {
    std::string response(30, '\0');
    EXPECT_THROW(peer::handshake_has_extensions(response), std::runtime_error);
}

} // namespace
