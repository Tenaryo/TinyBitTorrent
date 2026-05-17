# TinyBitTorrent

**A high-performance BitTorrent client from scratch in C++23 — 2,550 lines, 83 tests, 11 CLI commands.**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.21%2B-green)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-brightgreen)](./LICENSE)

## Features

| Category | Capabilities |
|---|---|
| **Bencode** | Full encoder & decoder with strict validation |
| **Torrent** | Parse `.torrent` files, extract info hash, piece hashes, file metadata |
| **Magnet Link** | Parse magnet URIs, resolve metadata via `ut_metadata` extension protocol |
| **Tracker** | HTTP tracker announce with compact peer list parsing |
| **Peer Protocol** | Handshake, bitfield exchange, interest/choke state machine |
| **Download** | Block-level pipelining (concurrent requests) with SHA-1 integrity verification |
| **Parallel** | Multi-peer concurrent download with connection reuse and `pwrite`-based zero-copy incremental I/O |

### Performance Optimizations

- **Pipeline depth 5** — up to 5 block requests in-flight per piece to saturate the link
- **Connection reuse** — single TCP handshake serves multiple pieces per peer
- **Multi-peer parallelism** — `std::jthread` workers download pieces concurrently for 2–5× speedup
- **Incremental writes** — `pwrite` places each piece directly at its file offset, avoiding entire-file buffering

### Modern C++

| Technique | Usage |
|---|---|
| C++23 | `std::ranges::copy`, `std::format`, `std::visit` + `Overloaded`, `std::jthread` |
| C++20 Attributes | `[[likely]]` / `[[unlikely]]` on protocol branching paths |
| RAII | `util::Socket` with move semantics (`noexcept`) |
| Type Safety | `std::variant`-based message dispatch instead of inheritance |
| Constexpr | Compile-time constants (`kBlockSize`, `kPipelineDepth`, `kCommands`) |
| Traits | C++17 deduction guides for `Overloaded` visitor pattern |

### Code Quality

| Tool | Role |
|---|---|
| `-Wall -Wextra -Wpedantic -Werror` | Zero-warning compilation |
| `-Wshadow -Wconversion` | Catches implicit narrowing / shadowing |
| `clang-format` | Consistent style via pre-commit hooks |
| `clang-tidy` | Static analysis with `bugprone-*`, `modernize-*`, `performance-*`, `readability-*` |
| `-fsanitize=address,undefined` | Optional ASan/UBSan for runtime safety |

## Quick Start

```bash
# Clone
git clone https://github.com/Tenaryo/TinyBitTorrent.git
cd TinyBitTorrent

# Build (requires CMake 3.21+, Ninja, C++23 compiler)
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Run tests
cd .. && ctest --test-dir build -j$(nproc)

# Download a file via magnet link
./bittorrent magnet_download -o ubuntu.iso "magnet:?xt=urn:btih:..."
```

## CLI Reference

```
bittorrent <command> [args]
```

### Torrent File Commands
| Command | Description |
|---|---|
| `info <torrent_file>` | Display torrent metadata (tracker, length, info hash, pieces) |
| `peers <torrent_file>` | Discover peers via HTTP tracker |
| `handshake <torrent_file> <ip:port>` | Perform BitTorrent handshake with a peer |
| `download_piece -o <output> <torrent_file> <index>` | Download a single piece |
| `download -o <output> <torrent_file>` | Download the complete file |

### Magnet Link Commands
| Command | Description |
|---|---|
| `magnet_parse <magnet_link>` | Parse and display magnet URI |
| `magnet_handshake <magnet_link>` | Extended handshake + extension negotiation |
| `magnet_info <magnet_link>` | Fetch torrent metadata via `ut_metadata` protocol |
| `magnet_download_piece -o <output> <magnet_link> <index>` | Download a piece via magnet link |
| `magnet_download -o <output> <magnet_link>` | Download the complete file via magnet link |

### Misc
| Command | Description |
|---|---|
| `decode <bencoded_string>` | Decode and pretty-print bencoded data |

## Architecture

```
main.cpp (entry point)
  └── cmd::dispatch() ──► Command Table (11 commands)
        │
        ├── magnet::parse()         ──► MagnetInfo {hash, tracker, name}
        ├── torrent::extract()      ──► Metainfo {length, pieces, hashes}
        ├── tracker::announce()     ──► HTTP GET ──► Peers[]
        │     └── http::get()
        │
        ├── peer::establish_connection()  ──► ReadyConnection (handshake + bitfield + unchoked)
        │     └── peer::download_piece_on_connection()
        │           └── Block-level pipelining ──► SHA-1 verify
        │
        └── download::download_file()
              └── std::jthread pool ──► pwrite to output file
```

## Project Structure

```
src/
├── main.cpp              # Entry point
├── cmd/                  # CLI command dispatch
├── bencode/              # Bencode encoder / decoder
├── torrent/              # .torrent file parsing
├── magnet/               # Magnet URI parsing
├── tracker/              # HTTP tracker client
├── peer/                 # Peer wire protocol + extensions
├── download/             # Multi-piece parallel download
├── http/                 # Raw HTTP client
├── output/               # Pretty-printing utilities
├── util/                 # SHA-1, TCP socket, random, URL encode
└── lib/                  # Third-party (nlohmann/json)
tests/
├── test_bencode.cpp      # 20 tests
├── test_message.cpp      # 28 tests
├── test_peer.cpp         # 17 tests
├── test_torrent.cpp      # 9 tests
├── test_sha1.cpp         # 5 tests
├── test_magnet.cpp       # 2 tests
└── test_e2e.cpp          # 2 tests
```

## License

MIT © 2026 Tenaryo
