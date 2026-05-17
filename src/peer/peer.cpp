#include "peer/peer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "bencode/decoder.hpp"
#include "bencode/encoder.hpp"
#include "bencode/value.hpp"
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

    if (msg_len == 0) [[unlikely]] {
        return recv_message(sock_fd);
    }
    if (msg_len < 0) [[unlikely]] {
        throw std::runtime_error("negative message length");
    }

    std::string body(static_cast<size_t>(msg_len), '\0');
    util::recv_all(sock_fd, std::span{body});
    return message::decode(body);
}

} // anonymous namespace

auto ext_handshake(int sock_fd,
                   std::string_view info_hash,
                   std::string_view our_peer_id,
                   uint8_t ext_id) -> MagnetHandshakeResult {
    auto hs_msg = make_handshake(info_hash, our_peer_id);
    util::send_all(sock_fd, hs_msg);

    std::array<char, 68> hs_buf{};
    util::recv_all(sock_fd, hs_buf);
    auto peer_id = parse_handshake_peer_id(
        std::string_view{hs_buf.data(), hs_buf.size()});

    bool has_ext = (static_cast<uint8_t>(hs_buf[25]) & 0x10) != 0;

    recv_message(sock_fd);

    if (has_ext) {
        bencode::Dict ext_dict;
        bencode::Dict inner_dict;
        inner_dict.items_.emplace_back("ut_metadata", bencode::Integer{ext_id});
        ext_dict.items_.emplace_back("m", std::move(inner_dict));

        auto ext_payload = bencode::encode(bencode::Value{ext_dict});
        auto ext_msg
            = message::encode(message::Extended{0, std::move(ext_payload)});
        util::send_all(sock_fd, ext_msg);

        auto msg = recv_message(sock_fd);
        const auto* ext_resp = std::get_if<message::Extended>(&msg);
        if (ext_resp == nullptr) [[unlikely]] {
            throw std::runtime_error("expected extended handshake response");
        }
        auto metadata_ext_id = parse_ext_handshake_response(ext_resp->payload_);
        return {std::move(peer_id), metadata_ext_id};
    }

    return {std::move(peer_id), std::nullopt};
}

auto magnet_handshake(std::string_view host,
                      uint16_t port,
                      std::string_view info_hash,
                      std::string_view our_peer_id,
                      uint8_t ext_id) -> MagnetHandshakeResult {
    util::Socket sock(host, port);
    return ext_handshake(sock.fd(), info_hash, our_peer_id, ext_id);
}

auto build_metadata_request(uint8_t peer_ext_id, int piece_index)
    -> std::string {
    bencode::Dict req;
    req.items_.emplace_back("msg_type", bencode::Integer{0});
    req.items_.emplace_back("piece", bencode::Integer{piece_index});

    auto payload = bencode::encode(bencode::Value{req});
    return message::encode(message::Extended{peer_ext_id, std::move(payload)});
}

auto send_metadata_request(int sock_fd, uint8_t peer_ext_id, int piece_index)
    -> void {
    auto msg = build_metadata_request(peer_ext_id, piece_index);
    util::send_all(sock_fd, msg);
}

auto magnet_info(std::string_view host,
                 uint16_t port,
                 std::string_view info_hash,
                 std::string_view our_peer_id,
                 uint8_t ext_id) -> torrent::Metainfo {
    util::Socket sock(host, port);
    auto result = ext_handshake(sock.fd(), info_hash, our_peer_id, ext_id);

    if (!result.metadata_ext_id_.has_value()) [[unlikely]] {
        throw std::runtime_error("peer does not support ut_metadata extension");
    }

    send_metadata_request(sock.fd(), *result.metadata_ext_id_, 0);

    auto msg = recv_message(sock.fd());
    const auto* ext_msg = std::get_if<message::Extended>(&msg);
    if (ext_msg == nullptr) {
        throw std::runtime_error(
            "expected extended message for metadata response");
    }
    if (ext_msg->ext_msg_id_ != ext_id) {
        throw std::runtime_error(
            "metadata response ext_msg_id mismatch: expected "
            + std::to_string(ext_id) + ", got "
            + std::to_string(ext_msg->ext_msg_id_));
    }

    auto info_dict_bencode = parse_metadata_data(ext_msg->payload_);
    auto info_value = bencode::decode(info_dict_bencode);
    const auto* info_dict = std::get_if<bencode::Dict>(&info_value);
    if (info_dict == nullptr) {
        throw std::runtime_error(
            "metadata piece contents is not a bencoded dictionary");
    }

    auto metainfo = torrent::from_info_dict(*info_dict);

    util::Sha1 hasher;
    hasher.update(info_dict_bencode);
    auto digest = hasher.finalize();
    std::string computed_hash(reinterpret_cast<const char*>(digest.data()),
                              digest.size());
    if (computed_hash != info_hash) [[unlikely]] {
        throw std::runtime_error("metadata info hash mismatch: expected "
                                 + util::bytes_to_hex(info_hash) + ", got "
                                 + util::bytes_to_hex(computed_hash));
    }

    return metainfo;
}

auto parse_metadata_data(std::string_view payload) -> std::string {
    auto value = bencode::decode(payload);
    const auto* dict = std::get_if<bencode::Dict>(&value);
    if (dict == nullptr) {
        throw std::runtime_error(
            "metadata response payload is not a bencoded dictionary");
    }

    const auto* msg_type_val = bencode::find(*dict, "msg_type");
    if (msg_type_val == nullptr) {
        throw std::runtime_error("metadata response missing 'msg_type' key");
    }
    const auto* msg_type = std::get_if<bencode::Integer>(msg_type_val);
    if (msg_type == nullptr || *msg_type != 1) {
        throw std::runtime_error(
            "metadata response has unexpected msg_type, expected 1");
    }

    const auto* piece_val = bencode::find(*dict, "piece");
    if (piece_val == nullptr) {
        throw std::runtime_error("metadata response missing 'piece' key");
    }
    const auto* piece = std::get_if<bencode::Integer>(piece_val);
    if (piece == nullptr || *piece != 0) {
        throw std::runtime_error(
            "metadata response has unexpected piece index, expected 0");
    }

    const auto* total_size_val = bencode::find(*dict, "total_size");
    if (total_size_val == nullptr) {
        throw std::runtime_error("metadata response missing 'total_size' key");
    }
    const auto* total_size = std::get_if<bencode::Integer>(total_size_val);
    if (total_size == nullptr || *total_size < 0) {
        throw std::runtime_error("metadata response has invalid total_size");
    }

    auto info_size = static_cast<std::size_t>(*total_size);
    if (info_size > payload.size()) {
        throw std::runtime_error("metadata total_size exceeds payload size");
    }

    return std::string(payload.substr(payload.size() - info_size));
}

auto parse_ext_handshake_response(std::string_view bencode_payload) -> uint8_t {
    auto value = bencode::decode(bencode_payload);
    const auto* dict = std::get_if<bencode::Dict>(&value);
    if (dict == nullptr) {
        throw std::runtime_error("extension handshake: payload is not a dict");
    }
    const auto* m_val = bencode::find(*dict, "m");
    if (m_val == nullptr) {
        throw std::runtime_error("extension handshake: missing 'm' key");
    }
    const auto* inner_dict = std::get_if<bencode::Dict>(m_val);
    if (inner_dict == nullptr) {
        throw std::runtime_error(
            "extension handshake: 'm' value is not a dict");
    }
    const auto* ut_val = bencode::find(*inner_dict, "ut_metadata");
    if (ut_val == nullptr) {
        throw std::runtime_error(
            "extension handshake: missing 'ut_metadata' key");
    }
    const auto* ext_id = std::get_if<bencode::Integer>(ut_val);
    if (ext_id == nullptr) {
        throw std::runtime_error(
            "extension handshake: 'ut_metadata' value is not an integer");
    }
    return static_cast<uint8_t>(*ext_id);
}

auto handshake_has_extensions(std::string_view response) -> bool {
    if (response.size() < 68) {
        throw std::runtime_error("handshake response too short");
    }
    return (static_cast<uint8_t>(response[25]) & 0x10) != 0;
}

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
        if (!std::holds_alternative<message::Bitfield>(msg)) [[unlikely]] {
            throw std::runtime_error("expected bitfield, got other message");
        }
        if (!message::has_piece(std::get<message::Bitfield>(msg), piece_index))
            [[unlikely]] {
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
        if (!std::holds_alternative<message::Unchoke>(msg)) [[unlikely]] {
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
                    if (send_idx < kTotalBlocks && pending < pipeline_depth)
                        [[likely]] {
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
    if (computed != expected) [[unlikely]] {
        throw std::runtime_error("piece hash mismatch: expected " + expected
                                 + ", got " + computed);
    }

    return piece_data;
}

} // namespace peer
