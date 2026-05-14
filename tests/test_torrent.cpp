#include <gtest/gtest.h>

#include "torrent/metainfo.hpp"

TEST(TorrentExtract, ExtractAnnounceAndLength) {
    bencode::Dict dict;
    dict.items_.push_back({"announce", bencode::String("http://example.com")});

    bencode::Dict info;
    info.items_.push_back({"length", bencode::Integer(12345)});
    info.items_.push_back({"piece length", bencode::Integer(32768)});
    info.items_.push_back({"pieces", bencode::String(std::string(20, '\x00'))});
    dict.items_.push_back({"info", info});

    auto result = torrent::extract(dict);
    EXPECT_EQ(result.announce_, "http://example.com");
    EXPECT_EQ(result.length_, 12345);
    EXPECT_EQ(result.info_hash_.size(), 20);
    EXPECT_FALSE(result.info_hash_.empty());
    EXPECT_EQ(result.piece_length_, 32768);
    ASSERT_EQ(result.piece_hashes_.size(), 1);
}

TEST(TorrentExtract, MissingAnnounceThrows) {
    bencode::Dict dict;
    bencode::Dict info;
    info.items_.push_back({"length", bencode::Integer(12345)});
    dict.items_.push_back({"info", info});

    EXPECT_THROW(torrent::extract(dict), std::runtime_error);
}

TEST(TorrentExtract, MissingInfoThrows) {
    bencode::Dict dict;
    dict.items_.push_back({"announce", bencode::String("http://example.com")});

    EXPECT_THROW(torrent::extract(dict), std::runtime_error);
}

TEST(TorrentExtract, MissingLengthThrows) {
    bencode::Dict dict;
    dict.items_.push_back({"announce", bencode::String("http://example.com")});
    dict.items_.push_back({"info", bencode::Dict{}});

    EXPECT_THROW(torrent::extract(dict), std::runtime_error);
}

TEST(TorrentExtract, ExtractPieceInfo) {
    bencode::Dict dict;
    dict.items_.push_back({"announce", bencode::String("http://example.com")});

    bencode::Dict info;
    info.items_.push_back({"length", bencode::Integer(12345)});
    info.items_.push_back({"piece length", bencode::Integer(32768)});

    std::string raw_pieces(40, '\0');
    raw_pieces[0] = '\xab';
    raw_pieces[19] = '\xcd';
    raw_pieces[20] = '\x12';
    raw_pieces[39] = '\x34';
    info.items_.push_back({"pieces", bencode::String(std::move(raw_pieces))});
    dict.items_.push_back({"info", std::move(info)});

    auto result = torrent::extract(dict);
    EXPECT_EQ(result.piece_length_, 32768);
    ASSERT_EQ(result.piece_hashes_.size(), 2);
    EXPECT_EQ(result.piece_hashes_[0],
              "ab000000000000000000000000000000000000cd");
    EXPECT_EQ(result.piece_hashes_[1],
              "1200000000000000000000000000000000000034");
}
