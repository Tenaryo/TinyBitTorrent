#include <gtest/gtest.h>

#include "torrent/metainfo.hpp"

TEST(TorrentExtract, ExtractAnnounceAndLength) {
    bencode::Dict dict;
    dict.items_.push_back({"announce", bencode::String("http://example.com")});

    bencode::Dict info;
    info.items_.push_back({"length", bencode::Integer(12345)});
    dict.items_.push_back({"info", info});

    auto result = torrent::extract(dict);
    EXPECT_EQ(result.announce_, "http://example.com");
    EXPECT_EQ(result.length_, 12345);
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
