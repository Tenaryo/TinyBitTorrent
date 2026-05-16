#pragma once

#include <string>
#include <string_view>

namespace magnet {

struct MagnetInfo {
    std::string info_hash_;
    std::string tracker_url_;
    std::string display_name_;
};

auto parse(std::string_view uri) -> MagnetInfo;

} // namespace magnet
