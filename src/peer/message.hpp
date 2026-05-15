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

using Message = std::variant<Interested, Unchoke, Bitfield, Request, Piece>;

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto encode(const Message& msg) -> std::string;
auto decode(std::string_view data) -> Message;

constexpr int32_t kBlockSize = 16 * 1024;
constexpr int kPipelineDepth = 5;

} // namespace peer::message
