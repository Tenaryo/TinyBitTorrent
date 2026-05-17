#include "download/download.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

#include "peer/peer.hpp"
#include "util/random.hpp"

namespace download {

void download_file(const torrent::Metainfo& metainfo,
                   const std::vector<tracker::Peer>& peers,
                   std::string_view output_path,
                   std::string_view peer_id) {
    if (peers.empty()) {
        throw std::runtime_error("no peers available");
    }

    // TODO: download pieces from multiple peers in parallel
    // TODO: reuse TCP connections across pieces to reduce handshake overhead
    // TODO: write pieces incrementally instead of buffering entire file

    std::string effective_peer_id;
    if (peer_id.empty()) {
        effective_peer_id = util::random_bytes(20);
        peer_id = effective_peer_id;
    }
    auto num_pieces = static_cast<int>(metainfo.piece_hashes_.size());

    std::string file_data(static_cast<size_t>(metainfo.length_), '\0');

    for (int piece_idx = 0; piece_idx < num_pieces; ++piece_idx) {
        std::string piece_data;
        bool downloaded = false;

        for (const auto& peer : peers) {
            try {
                piece_data = peer::download_piece(
                    metainfo, peer.ip_, peer.port_, peer_id, piece_idx);
                downloaded = true;
                break;
            } catch (const std::exception&) {
                continue;
            }
        }

        if (!downloaded) {
            throw std::runtime_error("failed to download piece "
                                     + std::to_string(piece_idx));
        }

        auto offset = static_cast<size_t>(piece_idx)
                      * static_cast<size_t>(metainfo.piece_length_);
        std::ranges::copy(piece_data,
                          file_data.begin()
                              + static_cast<std::ptrdiff_t>(offset));
    }

    std::ofstream out(output_path.data(), std::ios::binary);
    if (!out) {
        throw std::runtime_error(std::string{"cannot write to "}
                                 + std::string(output_path));
    }
    out.write(file_data.data(), static_cast<std::streamsize>(file_data.size()));
}

} // namespace download
