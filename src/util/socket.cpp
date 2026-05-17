#include "util/socket.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <format>
#include <stdexcept>
#include <string>

namespace util {

Socket::Socket(std::string_view host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    auto port_str = std::to_string(port);

    int ret = getaddrinfo(
        std::string{host}.c_str(), port_str.c_str(), &hints, &result);
    if (ret != 0) {
        throw std::runtime_error(
            std::format("getaddrinfo failed: {}", gai_strerror(ret)));
    }

    for (auto* rp = result; rp != nullptr; rp = rp->ai_next) {
        fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd_ < 0) {
            continue;
        }
        if (connect(fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close();
    }
    freeaddrinfo(result);

    if (fd_ < 0) {
        throw std::runtime_error(
            std::format("cannot connect to {}:{}", host, port));
    }
}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_{other.fd_} {
    other.fd_ = -1;
}

auto Socket::operator=(Socket&& other) noexcept -> Socket& {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

auto Socket::fd() const -> int {
    return fd_;
}

void Socket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void send_all(int sock_fd, std::string_view data) {
    while (!data.empty()) {
        ssize_t sent = ::send(sock_fd, data.data(), data.size(), 0);
        if (sent < 0) [[unlikely]] {
            throw std::runtime_error("send failed");
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

void recv_all(int sock_fd, std::span<char> buffer) {
    while (!buffer.empty()) {
        ssize_t received = ::recv(sock_fd, buffer.data(), buffer.size(), 0);
        if (received <= 0) [[unlikely]] {
            throw std::runtime_error("recv failed or connection closed");
        }
        buffer = buffer.subspan(static_cast<std::size_t>(received));
    }
}

} // namespace util
