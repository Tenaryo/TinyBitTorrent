#include "output/output.hpp"

#include <format>
#include <type_traits>
#include <variant>

#include "lib/nlohmann/json.hpp"

namespace output {

namespace {

auto value_to_json(const bencode::Value& value) -> nlohmann::json {
    return std::visit(
        [](const auto& val) -> nlohmann::json {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bencode::List>) {
                auto arr = nlohmann::json::array();
                for (const auto& elem : val.elements_) {
                    arr.push_back(value_to_json(elem));
                }
                return arr;
            } else if constexpr (std::is_same_v<T, bencode::Dict>) {
                auto obj = nlohmann::json::object();
                for (const auto& [key, entry] : val.items_) {
                    obj[key] = value_to_json(entry);
                }
                return obj;
            } else {
                return nlohmann::json(val);
            }
        },
        value);
}

} // anonymous namespace

auto format(const bencode::Value& value) -> std::string {
    return value_to_json(value).dump();
}

auto format(const torrent::Metainfo& info) -> std::string {
    return std::format(
        "Tracker URL: {}\nLength: {}\n", info.announce_, info.length_);
}

} // namespace output
