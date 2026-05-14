#pragma once

#include <string>

#include "bencode/value.hpp"
#include "torrent/metainfo.hpp"

namespace output {

auto format(const bencode::Value& value) -> std::string;

auto format(const torrent::Metainfo& info) -> std::string;

} // namespace output
