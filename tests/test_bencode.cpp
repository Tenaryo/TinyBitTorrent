#include <gtest/gtest.h>

#include "bencode/decoder.hpp"

TEST(BencodeDecode, NegativeInteger) {
    auto result = bencode::decode("i-52e");
    ASSERT_TRUE(std::holds_alternative<bencode::Integer>(result));
    EXPECT_EQ(std::get<bencode::Integer>(result), -52);
}

TEST(BencodeDecode, StringDecode) {
    auto result = bencode::decode("5:hello");
    ASSERT_TRUE(std::holds_alternative<bencode::String>(result));
    EXPECT_EQ(std::get<bencode::String>(result), "hello");
}
