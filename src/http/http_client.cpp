#include "http/http_client.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

namespace http {

namespace {

struct UrlParts {
    std::string_view host_;
    std::string_view path_;
    uint16_t port_;
};

auto parse_url(std::string_view url) -> UrlParts {
    if (!url.starts_with("http://")) {
        throw std::runtime_error("only http:// URLs are supported");
    }
    url.remove_prefix(7);

    auto slash = url.find('/');
    std::string_view hostport;
    std::string_view path;
    if (slash == std::string_view::npos) {
        hostport = url;
        path = "/";
    } else {
        hostport = url.substr(0, slash);
        path = url.substr(slash);
    }

    auto colon = hostport.find(':');
    std::string_view host;
    uint16_t port = 80;
    if (colon == std::string_view::npos) {
        host = hostport;
    } else {
        host = hostport.substr(0, colon);
        auto port_str = hostport.substr(colon + 1);
        int port_val = 0;
        auto [_, ec] = std::from_chars(
            port_str.data(), port_str.data() + port_str.size(), port_val);
        if (ec != std::errc{} || port_val < 1 || port_val > 65535) {
            throw std::runtime_error(std::format("invalid port: {}", port_str));
        }
        port = static_cast<uint16_t>(port_val);
    }

    return {host, path, port};
}

class Socket {
  public:
    Socket(std::string_view host, uint16_t port) {
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

    ~Socket() {
        close();
    }

    Socket(const Socket&) = delete;
    auto operator=(const Socket&) -> Socket& = delete;

    [[nodiscard]] auto fd() const -> int {
        return fd_;
    }

  private:
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    int fd_{-1};
};

auto send_all(int sock_fd, std::string_view data) -> void {
    while (!data.empty()) {
        ssize_t sent = ::send(sock_fd, data.data(), data.size(), 0);
        if (sent < 0) {
            throw std::runtime_error("send failed");
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

} // anonymous namespace

auto get(std::string_view url) -> std::string {
    auto [host, path, port] = parse_url(url);

    Socket sock(host, port);

    auto request = std::format(
        "GET {} HTTP/1.1\r\nHost: {}:{}\r\nConnection: close\r\n\r\n",
        path,
        host,
        port);
    send_all(sock.fd(), request);

    std::array<char, 4096> buf{};
    std::string response;
    while (true) {
        ssize_t received = recv(sock.fd(), buf.data(), buf.size(), 0);
        if (received < 0) {
            throw std::runtime_error("recv failed");
        }
        if (received == 0) {
            break;
        }
        response.append(buf.data(), static_cast<std::size_t>(received));
    }

    auto header_end = response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        throw std::runtime_error("invalid HTTP response: no header end");
    }

    auto status_line_end = response.find("\r\n");
    if (status_line_end == std::string::npos) {
        throw std::runtime_error("invalid HTTP response: no status line");
    }
    auto status_line = std::string_view{response}.substr(0, status_line_end);

    if (!status_line.starts_with("HTTP/1.")) {
        throw std::runtime_error(
            std::format("unexpected HTTP response: {}", status_line));
    }

    auto space1 = status_line.find(' ');
    if (space1 == std::string_view::npos) {
        throw std::runtime_error("invalid HTTP status line");
    }
    auto status = std::string_view{status_line}.substr(space1 + 1);
    if (!status.starts_with("200")) {
        throw std::runtime_error(
            std::format("HTTP server returned status: {}", status));
    }

    auto body = std::string_view{response}.substr(header_end + 4);
    return std::string{body};
}

} // namespace http
