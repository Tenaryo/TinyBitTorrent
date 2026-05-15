#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace util {

class Socket {
  public:
    Socket(std::string_view host, uint16_t port);
    ~Socket();

    Socket(const Socket&) = delete;
    auto operator=(const Socket&) -> Socket& = delete;

    Socket(Socket&& other) noexcept;
    auto operator=(Socket&& other) noexcept -> Socket&;

    [[nodiscard]] auto fd() const -> int;

  private:
    void close();
    int fd_{-1};
};

void send_all(int sock_fd, std::string_view data);
void recv_all(int sock_fd, std::span<char> buffer);

} // namespace util
