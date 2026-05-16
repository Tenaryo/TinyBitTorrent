#include "magnet/magnet.hpp"

#include <stdexcept>

#include "util/sha1.hpp"
#include "util/url_encode.hpp"

namespace magnet {

auto parse(std::string_view uri) -> MagnetInfo {
    constexpr std::string_view kPrefix = "magnet:?";
    if (!uri.starts_with(kPrefix)) {
        throw std::runtime_error(
            "invalid magnet link: missing 'magnet:?' prefix");
    }

    MagnetInfo info;
    bool has_xt = false;
    std::string_view tail = uri.substr(kPrefix.size());

    while (!tail.empty()) {
        auto eq_pos = tail.find('=');
        auto amp_pos = tail.find('&');
        auto val_end
            = amp_pos != std::string_view::npos ? amp_pos : tail.size();

        std::string_view key = tail.substr(0, eq_pos);
        std::string_view value = tail.substr(eq_pos + 1, val_end - eq_pos - 1);

        if (key == "xt") {
            constexpr std::string_view kXtPrefix = "urn:btih:";
            if (!value.starts_with(kXtPrefix)) {
                throw std::runtime_error(
                    "invalid xt value: expected 'urn:btih:<hash>'");
            }
            info.info_hash_
                = util::hex_to_bytes(value.substr(kXtPrefix.size()));
            has_xt = true;
        } else if (key == "tr") {
            info.tracker_url_ = util::url_decode(value);
        } else if (key == "dn") {
            info.display_name_ = value;
        }

        if (amp_pos == std::string_view::npos) {
            break;
        }
        tail = tail.substr(amp_pos + 1);
    }

    if (!has_xt) {
        throw std::runtime_error("missing required 'xt' parameter");
    }

    return info;
}

} // namespace magnet
