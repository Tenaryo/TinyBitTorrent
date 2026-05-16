#include "peer/message.hpp"

#include <stdexcept>

#include "util/bytes.hpp"

namespace peer::message {

auto encode(const Message& msg) -> std::string {
    return std::visit(
        Overloaded{
            [](const Interested&) -> std::string { return {0, 0, 0, 1, 2}; },
            [](const Unchoke&) -> std::string { return {0, 0, 0, 1, 1}; },
            [](const Bitfield& bfld) -> std::string {
                std::string buf;
                auto payload_len = 1 + static_cast<int32_t>(bfld.data_.size());
                util::write_int32_be(buf, payload_len);
                buf.push_back(5);
                buf.append(bfld.data_);
                return buf;
            },
            [](const Request& req) -> std::string {
                std::string buf;
                util::write_int32_be(buf, 13);
                buf.push_back(6);
                util::write_int32_be(buf, req.index_);
                util::write_int32_be(buf, req.begin_);
                util::write_int32_be(buf, req.length_);
                return buf;
            },
            [](const Piece& pce) -> std::string {
                std::string buf;
                auto payload_len
                    = 1 + 8 + static_cast<int32_t>(pce.block_.size());
                util::write_int32_be(buf, payload_len);
                buf.push_back(7);
                util::write_int32_be(buf, pce.index_);
                util::write_int32_be(buf, pce.begin_);
                buf.append(pce.block_);
                return buf;
            },
            [](const Extended& ext) -> std::string {
                std::string buf;
                auto payload_len
                    = 1 + 1 + static_cast<int32_t>(ext.payload_.size());
                util::write_int32_be(buf, payload_len);
                buf.push_back(20);
                buf.push_back(static_cast<char>(ext.ext_msg_id_));
                buf.append(ext.payload_);
                return buf;
            },
        },
        msg);
}

auto decode(std::string_view data) -> Message {
    if (data.empty()) {
        throw std::runtime_error("empty message payload");
    }
    auto msg_id = static_cast<uint8_t>(data[0]);
    auto payload = data.substr(1);

    switch (msg_id) {
    case 1:
        return Unchoke{};
    case 2:
        return Interested{};
    case 5: {
        Bitfield bfld;
        bfld.data_ = std::string(payload);
        return bfld;
    }
    case 6: {
        const auto* ptr = payload.data();
        Request req{};
        req.index_ = util::read_int32_be(ptr);
        req.begin_ = util::read_int32_be(ptr);
        req.length_ = util::read_int32_be(ptr);
        return req;
    }
    case 7: {
        const auto* ptr = payload.data();
        Piece pce{};
        pce.index_ = util::read_int32_be(ptr);
        pce.begin_ = util::read_int32_be(ptr);
        pce.block_ = std::string(payload.substr(8));
        return pce;
    }
    case 20: {
        if (payload.empty()) {
            throw std::runtime_error("extended message too short");
        }
        Extended ext{};
        ext.ext_msg_id_ = static_cast<uint8_t>(payload[0]);
        ext.payload_ = std::string(payload.substr(1));
        return ext;
    }
    default:
        throw std::runtime_error("unknown message id: "
                                 + std::to_string(msg_id));
    }
}

auto has_piece(const Bitfield& bfld, int piece_index) -> bool {
    auto byte_idx = piece_index / 8;
    if (byte_idx >= static_cast<int>(bfld.data_.size())) {
        return false;
    }
    auto bit_idx = 7 - (piece_index % 8);
    return (static_cast<uint8_t>(bfld.data_[static_cast<size_t>(byte_idx)])
            & (1 << bit_idx))
           != 0;
}

} // namespace peer::message
