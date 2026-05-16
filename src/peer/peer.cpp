#include "peer/peer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "peer/message.hpp"
#include "util/bytes.hpp"
#include "util/sha1.hpp"
#include "util/socket.hpp"

namespace peer {

auto make_handshake(std::string_view info_hash, std::string_view our_peer_id)
    -> std::string {
    std::string msg(68, '\0');
    msg[0] = 19;
    constexpr std::string_view kProtocol = "BitTorrent protocol";
    std::ranges::copy(kProtocol, msg.begin() + 1);
    msg[25] = '\x10';
    std::ranges::copy(info_hash, msg.begin() + 28);
    std::ranges::copy(our_peer_id, msg.begin() + 48);
    return msg;
}

auto parse_handshake_peer_id(std::string_view response) -> std::string {
    if (response.size() < 68) {
        throw std::runtime_error("handshake response too short");
    }
    if (response[0] != 19) {
        throw std::runtime_error("invalid protocol string length");
    }
    if (response.substr(1, 19) != "BitTorrent protocol") {
        throw std::runtime_error("invalid protocol string");
    }
    return std::string{response.substr(48, 20)};
}

auto handshake(std::string_view host,
               uint16_t port,
               std::string_view info_hash,
               std::string_view our_peer_id) -> std::string {
    util::Socket sock(host, port);
    auto msg = make_handshake(info_hash, our_peer_id);
    util::send_all(sock.fd(), msg);
    std::array<char, 68> buf{};
    util::recv_all(sock.fd(), buf);
    return parse_handshake_peer_id(std::string_view{buf.data(), buf.size()});
}

namespace {

auto recv_message(int sock_fd) -> message::Message {
    std::array<char, 4> len_buf{};
    util::recv_all(sock_fd, std::span{len_buf});
    const char* ptr = len_buf.data();
    auto msg_len = util::read_int32_be(ptr);

    if (msg_len == 0) {
        return recv_message(sock_fd);
    }
    if (msg_len < 0) {
        throw std::runtime_error("negative message length");
    }

    std::string body(static_cast<size_t>(msg_len), '\0');
    util::recv_all(sock_fd, std::span{body});
    return message::decode(body);
}

} // anonymous namespace

auto download_piece(const torrent::Metainfo& info,
                    std::string_view peer_ip,
                    uint16_t peer_port,
                    std::string_view our_peer_id,
                    int piece_index,
                    int pipeline_depth) -> std::string {
    util::Socket sock(peer_ip, peer_port);

    auto handshake_msg = make_handshake(info.info_hash_, our_peer_id);
    util::send_all(sock.fd(), handshake_msg);
    std::array<char, 68> hs_buf{};
    util::recv_all(sock.fd(), hs_buf);
    parse_handshake_peer_id(std::string_view{hs_buf.data(), hs_buf.size()});

    {
        auto msg = recv_message(sock.fd());
        if (!std::holds_alternative<message::Bitfield>(msg)) {
            throw std::runtime_error("expected bitfield, got other message");
        }
        if (!message::has_piece(std::get<message::Bitfield>(msg),
                                piece_index)) {
            throw std::runtime_error("peer does not have piece "
                                     + std::to_string(piece_index));
        }
    }

    {
        auto interested = message::encode(message::Interested{});
        util::send_all(sock.fd(), interested);
    }

    {
        auto msg = recv_message(sock.fd());
        if (!std::holds_alternative<message::Unchoke>(msg)) {
            throw std::runtime_error("expected unchoke, got other message");
        }
    }

    auto num_pieces
        = (info.length_ + info.piece_length_ - 1) / info.piece_length_;
    auto actual_piece_len = static_cast<int32_t>(info.piece_length_);
    if (static_cast<int64_t>(piece_index) == num_pieces - 1) {
        auto remainder = info.length_ % info.piece_length_;
        if (remainder != 0) {
            actual_piece_len = static_cast<int32_t>(remainder);
        }
    }

    struct Block {
        int32_t begin_;
        int32_t length_;
    };
    std::vector<Block> blocks;
    for (int32_t off = 0; off < actual_piece_len; off += message::kBlockSize) {
        auto blen = std::min(message::kBlockSize, actual_piece_len - off);
        blocks.push_back({off, blen});
    }

    const auto kTotalBlocks = blocks.size();
    std::vector<bool> received(kTotalBlocks, false);
    std::string piece_data(static_cast<size_t>(actual_piece_len), '\0');
    size_t blocks_received = 0;
    size_t send_idx = 0;
    int pending = 0;

    while (pending < pipeline_depth && send_idx < kTotalBlocks) {
        auto& blk = blocks[send_idx];
        auto req = message::encode(
            message::Request{piece_index, blk.begin_, blk.length_});
        util::send_all(sock.fd(), req);
        ++send_idx;
        ++pending;
    }

    while (blocks_received < kTotalBlocks) {
        auto msg = recv_message(sock.fd());
        std::visit(
            message::Overloaded{
                [&](const message::Piece& pce) {
                    for (size_t i = 0; i < kTotalBlocks; ++i) {
                        if (!received[i] && blocks[i].begin_ == pce.begin_) {
                            std::ranges::copy(pce.block_,
                                              piece_data.begin() + pce.begin_);
                            received[i] = true;
                            ++blocks_received;
                            --pending;
                            break;
                        }
                    }
                    if (send_idx < kTotalBlocks && pending < pipeline_depth) {
                        auto& blk = blocks[send_idx];
                        auto req = message::encode(message::Request{
                            piece_index, blk.begin_, blk.length_});
                        util::send_all(sock.fd(), req);
                        ++send_idx;
                        ++pending;
                    }
                },
                [](const message::Bitfield&) {},
                [](const message::Unchoke&) {},
                [](const auto&) {},
            },
            msg);
    }

    auto computed = util::sha1_hex(piece_data);
    auto expected = info.piece_hashes_[static_cast<size_t>(piece_index)];
    if (computed != expected) {
        throw std::runtime_error("piece hash mismatch: expected " + expected
                                 + ", got " + computed);
    }

    return piece_data;
}

} // namespace peer
