#include <gtest/gtest.h>

#include "util/sha1.hpp"

TEST(Sha1, EmptyString) {
    EXPECT_EQ(util::sha1_hex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(Sha1, Fox) {
    EXPECT_EQ(util::sha1_hex("The quick brown fox jumps over the lazy dog"),
              "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

TEST(Sha1, Abc) {
    EXPECT_EQ(util::sha1_hex("abc"),
              "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST(Sha1, MessageDigest) {
    EXPECT_EQ(util::sha1_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmno"
                             "mnopnopq"),
              "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST(Sha1, IncrementalUpdate) {
    util::Sha1 h;
    h.update("abc");
    h.update("d");
    auto expected = [] {
        util::Sha1 h2;
        h2.update("abcd");
        return h2.finalize();
    }();
    EXPECT_EQ(h.finalize(), expected);
}
