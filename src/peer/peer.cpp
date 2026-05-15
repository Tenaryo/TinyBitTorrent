#include "peer/peer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "util/socket.hpp"

namespace peer {

auto make_handshake(std::string_view info_hash, std::string_view our_peer_id)
    -> std::string {
    std::string msg(68, '\0');
    msg[0] = 19;
    constexpr std::string_view kProtocol = "BitTorrent protocol";
    std::ranges::copy(kProtocol, msg.begin() + 1);
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

} // namespace peer
