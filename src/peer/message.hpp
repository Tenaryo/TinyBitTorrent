#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace peer::message {

struct Interested {};
struct Unchoke {};
struct Bitfield {
    std::string data_;
};
struct Request {
    int32_t index_;
    int32_t begin_;
    int32_t length_;
};
struct Piece {
    int32_t index_;
    int32_t begin_;
    std::string block_;
};
struct Extended {
    uint8_t ext_msg_id_;
    std::string payload_;
};

using Message
    = std::variant<Interested, Unchoke, Bitfield, Request, Piece, Extended>;

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto encode(const Message& msg) -> std::string;
auto decode(std::string_view data) -> Message;
auto has_piece(const Bitfield& bfld, int piece_index) -> bool;

constexpr int32_t kBlockSize = 16 * 1024;
constexpr int kPipelineDepth = 5;

} // namespace peer::message
