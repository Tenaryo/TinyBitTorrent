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

TEST(BencodeDecode, SimpleList) {
    auto result = bencode::decode("l5:helloi52ee");
    ASSERT_TRUE(std::holds_alternative<bencode::List>(result));

    const auto& list = std::get<bencode::List>(result);
    ASSERT_EQ(list.elements_.size(), 2);

    ASSERT_TRUE(std::holds_alternative<bencode::String>(list.elements_[0]));
    EXPECT_EQ(std::get<bencode::String>(list.elements_[0]), "hello");

    ASSERT_TRUE(std::holds_alternative<bencode::Integer>(list.elements_[1]));
    EXPECT_EQ(std::get<bencode::Integer>(list.elements_[1]), 52);
}

TEST(BencodeDecode, EmptyList) {
    auto result = bencode::decode("le");
    ASSERT_TRUE(std::holds_alternative<bencode::List>(result));

    const auto& list = std::get<bencode::List>(result);
    EXPECT_TRUE(list.elements_.empty());
}

TEST(BencodeDecode, StringList) {
    auto result = bencode::decode("l3:foo3:bare");
    ASSERT_TRUE(std::holds_alternative<bencode::List>(result));

    const auto& list = std::get<bencode::List>(result);
    ASSERT_EQ(list.elements_.size(), 2);

    ASSERT_TRUE(std::holds_alternative<bencode::String>(list.elements_[0]));
    EXPECT_EQ(std::get<bencode::String>(list.elements_[0]), "foo");

    ASSERT_TRUE(std::holds_alternative<bencode::String>(list.elements_[1]));
    EXPECT_EQ(std::get<bencode::String>(list.elements_[1]), "bar");
}
