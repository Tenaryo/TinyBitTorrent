#include <gtest/gtest.h>

#include "bencode/decoder.hpp"
#include "bencode/encoder.hpp"

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

TEST(BencodeDecode, EmptyDict) {
    auto result = bencode::decode("de");
    ASSERT_TRUE(std::holds_alternative<bencode::Dict>(result));

    const auto& dict = std::get<bencode::Dict>(result);
    EXPECT_TRUE(dict.items_.empty());
}

TEST(BencodeDecode, SimpleDict) {
    auto result = bencode::decode("d3:foo3:bar5:helloi52ee");
    ASSERT_TRUE(std::holds_alternative<bencode::Dict>(result));

    const auto& dict = std::get<bencode::Dict>(result);
    ASSERT_EQ(dict.items_.size(), 2);

    EXPECT_EQ(dict.items_[0].first, "foo");
    ASSERT_TRUE(std::holds_alternative<bencode::String>(dict.items_[0].second));
    EXPECT_EQ(std::get<bencode::String>(dict.items_[0].second), "bar");

    EXPECT_EQ(dict.items_[1].first, "hello");
    ASSERT_TRUE(
        std::holds_alternative<bencode::Integer>(dict.items_[1].second));
    EXPECT_EQ(std::get<bencode::Integer>(dict.items_[1].second), 52);
}

TEST(BencodeDecode, NestedDict) {
    auto result = bencode::decode("d4:datad3:key5:valueee");
    ASSERT_TRUE(std::holds_alternative<bencode::Dict>(result));

    const auto& dict = std::get<bencode::Dict>(result);
    ASSERT_EQ(dict.items_.size(), 1);

    EXPECT_EQ(dict.items_[0].first, "data");
    ASSERT_TRUE(std::holds_alternative<bencode::Dict>(dict.items_[0].second));

    const auto& inner = std::get<bencode::Dict>(dict.items_[0].second);
    ASSERT_EQ(inner.items_.size(), 1);

    EXPECT_EQ(inner.items_[0].first, "key");
    ASSERT_TRUE(
        std::holds_alternative<bencode::String>(inner.items_[0].second));
    EXPECT_EQ(std::get<bencode::String>(inner.items_[0].second), "value");
}

TEST(BencodeEncode, RoundTripString) {
    auto encoded = bencode::encode(bencode::String("hello"));
    auto decoded = bencode::decode(encoded);
    ASSERT_TRUE(std::holds_alternative<bencode::String>(decoded));
    EXPECT_EQ(std::get<bencode::String>(decoded), "hello");
}

TEST(BencodeEncode, RoundTripInteger) {
    auto encoded = bencode::encode(bencode::Integer(-52));
    auto decoded = bencode::decode(encoded);
    ASSERT_TRUE(std::holds_alternative<bencode::Integer>(decoded));
    EXPECT_EQ(std::get<bencode::Integer>(decoded), -52);
}

TEST(BencodeEncode, RoundTripList) {
    bencode::List list;
    list.elements_.push_back(bencode::String("hello"));
    list.elements_.push_back(bencode::Integer(52));
    auto encoded = bencode::encode(list);
    auto decoded = bencode::decode(encoded);
    ASSERT_TRUE(std::holds_alternative<bencode::List>(decoded));
    const auto& result = std::get<bencode::List>(decoded);
    ASSERT_EQ(result.elements_.size(), 2);
    ASSERT_TRUE(std::holds_alternative<bencode::String>(result.elements_[0]));
    EXPECT_EQ(std::get<bencode::String>(result.elements_[0]), "hello");
    ASSERT_TRUE(std::holds_alternative<bencode::Integer>(result.elements_[1]));
    EXPECT_EQ(std::get<bencode::Integer>(result.elements_[1]), 52);
}

TEST(BencodeEncode, RoundTripDict) {
    auto encoded = bencode::encode(bencode::decode("d3:foo3:bar5:helloi52ee"));
    EXPECT_EQ(encoded, "d3:foo3:bar5:helloi52ee");
}

TEST(BencodeEncode, RoundTripNestedDict) {
    auto encoded = bencode::encode(bencode::decode("d4:datad3:key5:valueee"));
    EXPECT_EQ(encoded, "d4:datad3:key5:valueee");
}

TEST(BencodeEncode, DictKeySorting) {
    bencode::Dict dict;
    dict.items_.push_back({"z", bencode::Integer(1)});
    dict.items_.push_back({"a", bencode::Integer(2)});
    auto encoded = bencode::encode(dict);
    EXPECT_EQ(encoded, "d1:ai2e1:zi1ee");
}

TEST(BencodeEncode, EmptyList) {
    EXPECT_EQ(bencode::encode(bencode::List{}), "le");
}

TEST(BencodeEncode, EmptyDict) {
    EXPECT_EQ(bencode::encode(bencode::Dict{}), "de");
}
