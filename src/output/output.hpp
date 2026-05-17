#pragma once

#include <string>

#include "bencode/value.hpp"
#include "magnet/magnet.hpp"
#include "peer/peer.hpp"
#include "torrent/metainfo.hpp"

namespace output {

auto format(const bencode::Value& value) -> std::string;

auto format(const torrent::Metainfo& info) -> std::string;

auto format(const magnet::MagnetInfo& info) -> std::string;

auto format(const peer::MagnetHandshakeResult& result) -> std::string;

} // namespace output
