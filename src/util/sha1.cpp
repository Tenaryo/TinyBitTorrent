#include "util/sha1.hpp"

#include <bit>
#include <cstring>

namespace util {

namespace {

constexpr auto rotl(uint32_t val, int shift) -> uint32_t {
    return (val << shift) | (val >> (32 - shift));
}

} // anonymous namespace

Sha1::Sha1()
    : h_{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0}, block_{},
      block_len_(0), total_len_(0) {
}

void Sha1::update(std::string_view data) {
    total_len_ += data.size();

    while (!data.empty()) {
        auto space = block_.size() - block_len_;
        auto chunk = std::min(space, data.size());
        std::memcpy(block_.data() + block_len_, data.data(), chunk);
        block_len_ += chunk;
        data.remove_prefix(chunk);

        if (block_len_ == block_.size()) {
            process_block();
            block_len_ = 0;
        }
    }
}

auto Sha1::finalize() -> std::array<uint8_t, 20> {
    uint64_t bit_len = total_len_ * 8;

    block_[block_len_++] = 0x80;

    if (block_len_ > 56) {
        std::memset(block_.data() + block_len_, 0, block_.size() - block_len_);
        process_block();
        block_len_ = 0;
    }

    std::memset(block_.data() + block_len_, 0, 56 - block_len_);

    for (std::size_t pos = 0; pos < 8; ++pos) {
        block_[56 + pos] = static_cast<uint8_t>(bit_len >> (56 - pos * 8));
    }

    process_block();

    std::array<uint8_t, 20> digest{};
    for (std::size_t idx = 0; idx < 5; ++idx) {
        uint32_t word = std::byteswap(h_[idx]);
        std::memcpy(digest.data() + idx * 4, &word, 4);
    }

    return digest;
}

void Sha1::process_block() {
    std::array<uint32_t, 80> schedule{};

    for (std::size_t idx = 0; idx < 16; ++idx) {
        auto base = idx * 4;
        schedule[idx] = static_cast<uint32_t>(block_[base]) << 24
                        | static_cast<uint32_t>(block_[base + 1]) << 16
                        | static_cast<uint32_t>(block_[base + 2]) << 8
                        | static_cast<uint32_t>(block_[base + 3]);
    }

    for (std::size_t idx = 16; idx < 80; ++idx) {
        schedule[idx] = rotl(schedule[idx - 3] ^ schedule[idx - 8]
                                 ^ schedule[idx - 14] ^ schedule[idx - 16],
                             1);
    }

    uint32_t state_a = h_[0];
    uint32_t state_b = h_[1];
    uint32_t state_c = h_[2];
    uint32_t state_d = h_[3];
    uint32_t state_e = h_[4];

    for (std::size_t idx = 0; idx < 80; ++idx) {
        uint32_t func_val;
        uint32_t round_const;
        if (idx < 20) {
            func_val = (state_b & state_c) | (~state_b & state_d);
            round_const = 0x5a827999;
        } else if (idx < 40) {
            func_val = state_b ^ state_c ^ state_d;
            round_const = 0x6ed9eba1;
        } else if (idx < 60) {
            func_val = (state_b & state_c) | (state_b & state_d)
                       | (state_c & state_d);
            round_const = 0x8f1bbcdc;
        } else {
            func_val = state_b ^ state_c ^ state_d;
            round_const = 0xca62c1d6;
        }

        uint32_t temp = rotl(state_a, 5) + func_val + state_e + round_const
                        + schedule[idx];
        state_e = state_d;
        state_d = state_c;
        state_c = rotl(state_b, 30);
        state_b = state_a;
        state_a = temp;
    }

    h_[0] += state_a;
    h_[1] += state_b;
    h_[2] += state_c;
    h_[3] += state_d;
    h_[4] += state_e;
}

} // namespace util
