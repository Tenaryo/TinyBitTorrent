#pragma once

#include <string_view>
#include <vector>

#include "torrent/metainfo.hpp"
#include "tracker/tracker.hpp"

namespace download {

void download_file(const torrent::Metainfo& metainfo,
                   const std::vector<tracker::Peer>& peers,
                   std::string_view output_path,
                   std::string_view peer_id = {});

} // namespace download
