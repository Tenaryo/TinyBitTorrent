#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "peer/message.hpp"
#include "util/bytes.hpp"
#include "util/sha1.hpp"

namespace {

TEST(Message, EncodeDecodeInterested) {
    using namespace peer::message;
    auto encoded = encode(Interested{});
    std::string expected = {0, 0, 0, 1, 2};
    EXPECT_EQ(encoded, expected);

    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Interested>(decoded));
}

TEST(Message, EncodeDecodeUnchoke) {
    using namespace peer::message;
    auto encoded = encode(Unchoke{});
    std::string expected = {0, 0, 0, 1, 1};
    EXPECT_EQ(encoded, expected);

    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Unchoke>(decoded));
}

TEST(Message, EncodeDecodeBitfield) {
    using namespace peer::message;
    Bitfield bf;
    bf.data_ = std::string{'\x01', '\x02', '\x03'};
    auto encoded = encode(bf);
    ASSERT_GE(encoded.size(), 5);
    EXPECT_EQ(encoded[4], 5); // message id
    EXPECT_EQ(encoded[5], '\x01');
    EXPECT_EQ(encoded[6], '\x02');
    EXPECT_EQ(encoded[7], '\x03');

    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Bitfield>(decoded));
    EXPECT_EQ(std::get<Bitfield>(decoded).data_, bf.data_);
}

TEST(Message, EncodeDecodeRequest) {
    using namespace peer::message;
    Request req{2, 16384, 16384};
    auto encoded = encode(req);
    ASSERT_GE(encoded.size(), 5);
    EXPECT_EQ(encoded[4], 6); // message id

    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Request>(decoded));
    auto& req_dec = std::get<Request>(decoded);
    EXPECT_EQ(req_dec.index_, 2);
    EXPECT_EQ(req_dec.begin_, 16384);
    EXPECT_EQ(req_dec.length_, 16384);
}

TEST(Message, EncodeDecodeRequestLastBlockShort) {
    using namespace peer::message;
    Request req{0, 49152, 848};
    auto encoded = encode(req);
    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Request>(decoded));
    auto& req_dec = std::get<Request>(decoded);
    EXPECT_EQ(req_dec.index_, 0);
    EXPECT_EQ(req_dec.begin_, 49152);
    EXPECT_EQ(req_dec.length_, 848);
}

TEST(Message, EncodeDecodePiece) {
    using namespace peer::message;
    Piece pce{0, 16384, std::string(100, '\xAB')};
    auto encoded = encode(pce);
    ASSERT_GE(encoded.size(), 5);
    EXPECT_EQ(encoded[4], 7); // message id

    auto decoded = decode(std::string_view{encoded}.substr(4));
    ASSERT_TRUE(std::holds_alternative<Piece>(decoded));
    auto& pp = std::get<Piece>(decoded);
    EXPECT_EQ(pp.index_, 0);
    EXPECT_EQ(pp.begin_, 16384);
    EXPECT_EQ(pp.block_.size(), 100);
    EXPECT_EQ(pp.block_, std::string(100, '\xAB'));
}

TEST(Message, DecodeRejectsEmptyPayload) {
    EXPECT_THROW(peer::message::decode(""), std::runtime_error);
}

TEST(Message, DecodeRejectsUnknownId) {
    EXPECT_THROW(peer::message::decode(std::string{'\x0F'}),
                 std::runtime_error);
}

TEST(PieceBlocking, EvenBlocks) {
    const int32_t piece_length = 65536;
    const int32_t block_size = 16 * 1024;
    std::vector<std::pair<int32_t, int32_t>> blocks;
    for (int32_t off = 0; off < piece_length; off += block_size) {
        auto blen = std::min(block_size, piece_length - off);
        blocks.emplace_back(off, blen);
    }
    ASSERT_EQ(blocks.size(), 4);
    EXPECT_EQ(blocks[0], (std::pair<int32_t, int32_t>{0, 16384}));
    EXPECT_EQ(blocks[1], (std::pair<int32_t, int32_t>{16384, 16384}));
    EXPECT_EQ(blocks[2], (std::pair<int32_t, int32_t>{32768, 16384}));
    EXPECT_EQ(blocks[3], (std::pair<int32_t, int32_t>{49152, 16384}));
}

TEST(PieceBlocking, UnevenLastBlock) {
    const int32_t piece_length = 50000;
    const int32_t block_size = 16 * 1024;
    std::vector<std::pair<int32_t, int32_t>> blocks;
    for (int32_t off = 0; off < piece_length; off += block_size) {
        auto blen = std::min(block_size, piece_length - off);
        blocks.emplace_back(off, blen);
    }
    ASSERT_EQ(blocks.size(), 4);
    EXPECT_EQ(blocks[1], (std::pair<int32_t, int32_t>{16384, 16384}));
    EXPECT_EQ(blocks[2], (std::pair<int32_t, int32_t>{32768, 16384}));
    EXPECT_EQ(blocks[3], (std::pair<int32_t, int32_t>{49152, 848}));
}

TEST(PieceBlocking, SingleBlock) {
    const int32_t piece_length = 16384;
    const int32_t block_size = 16 * 1024;
    std::vector<std::pair<int32_t, int32_t>> blocks;
    for (int32_t off = 0; off < piece_length; off += block_size) {
        auto blen = std::min(block_size, piece_length - off);
        blocks.emplace_back(off, blen);
    }
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0], (std::pair<int32_t, int32_t>{0, 16384}));
}

TEST(PieceBlocking, SmallPiece) {
    const int32_t piece_length = 500;
    const int32_t block_size = 16 * 1024;
    std::vector<std::pair<int32_t, int32_t>> blocks;
    for (int32_t off = 0; off < piece_length; off += block_size) {
        auto blen = std::min(block_size, piece_length - off);
        blocks.emplace_back(off, blen);
    }
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0], (std::pair<int32_t, int32_t>{0, 500}));
}

TEST(PieceHash, KnownValueSha1) {
    std::string data = "hello world";
    auto computed = util::sha1_hex(data);
    EXPECT_EQ(computed, "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
}

TEST(PieceHash, MismatchDetected) {
    std::string data = "hello world";
    auto computed = util::sha1_hex(data);
    EXPECT_NE(computed, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
}

TEST(PieceHash, EmptyData) {
    std::string empty;
    auto computed = util::sha1_hex(empty);
    EXPECT_EQ(computed, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(BytesUtil, WriteReadInt32Be) {
    std::string buf;
    util::write_int32_be(buf, 65536);
    ASSERT_EQ(buf.size(), 4);
    const char* ptr = buf.data();
    EXPECT_EQ(util::read_int32_be(ptr), 65536);
}

TEST(BytesUtil, WriteReadNegativeInt32) {
    std::string buf;
    util::write_int32_be(buf, -1);
    ASSERT_EQ(buf.size(), 4);
    const char* ptr = buf.data();
    EXPECT_EQ(util::read_int32_be(ptr), -1);
}

} // anonymous namespace
