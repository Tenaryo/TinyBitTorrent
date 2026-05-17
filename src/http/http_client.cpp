#include "http/http_client.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <stdexcept>
#include <string_view>

#include "util/socket.hpp"

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

} // anonymous namespace

auto get(std::string_view url) -> std::string {
    auto [host, path, port] = parse_url(url);

    util::Socket sock(host, port);

    auto request = std::format(
        "GET {} HTTP/1.1\r\nHost: {}:{}\r\nConnection: close\r\n\r\n",
        path,
        host,
        port);
    util::send_all(sock.fd(), request);

    std::array<char, 4096> buf{};
    std::string response;
    while (true) {
        ssize_t received = recv(sock.fd(), buf.data(), buf.size(), 0);
        if (received < 0) [[unlikely]] {
            throw std::runtime_error("recv failed");
        }
        if (received == 0) [[unlikely]] {
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
