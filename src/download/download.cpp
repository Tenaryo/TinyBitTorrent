#include "download/download.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

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

    std::string effective_peer_id;
    if (peer_id.empty()) {
        effective_peer_id = util::random_bytes(20);
        peer_id = effective_peer_id;
    }

    auto num_pieces = static_cast<int>(metainfo.piece_hashes_.size());
    auto num_workers = std::min(peers.size(), static_cast<size_t>(num_pieces));

    int file_fd
        = ::open(output_path.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) {
        throw std::runtime_error(std::string{"cannot open file: "}
                                 + std::string(output_path));
    }

    if (::ftruncate(file_fd, static_cast<off_t>(metainfo.length_)) < 0) {
        ::close(file_fd);
        throw std::runtime_error("failed to preallocate output file");
    }

    std::vector<std::jthread> workers;
    workers.reserve(num_workers);
    std::exception_ptr first_error;
    std::mutex error_mutex;

    auto base = num_pieces / static_cast<int>(num_workers);
    auto rem = num_pieces % static_cast<int>(num_workers);

    for (size_t wi = 0; wi < num_workers; ++wi) {
        int start
            = static_cast<int>(wi) * base + std::min(static_cast<int>(wi), rem);
        int end = start + base + (static_cast<int>(wi) < rem ? 1 : 0);

        workers.emplace_back([&metainfo,
                              &peers,
                              wi,
                              start,
                              end,
                              file_fd,
                              peer_id,
                              &first_error,
                              &error_mutex,
                              piece_length = metainfo.piece_length_]() {
            try {
                auto conn = peer::establish_connection(peers[wi].ip_,
                                                       peers[wi].port_,
                                                       metainfo.info_hash_,
                                                       peer_id);

                for (int pi = start; pi < end; ++pi) {
                    auto data = peer::download_piece_on_connection(
                        conn, metainfo, pi);

                    auto offset = static_cast<off_t>(pi)
                                  * static_cast<off_t>(piece_length);
                    auto written = ::pwrite(file_fd,
                                            data.data(),
                                            static_cast<size_t>(data.size()),
                                            offset);
                    if (written != static_cast<ssize_t>(data.size())) {
                        throw std::runtime_error("pwrite failed for piece "
                                                 + std::to_string(pi));
                    }
                }
            } catch (...) {
                std::lock_guard lock(error_mutex);
                if (!first_error) {
                    first_error = std::current_exception();
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    ::close(file_fd);

    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

} // namespace download
