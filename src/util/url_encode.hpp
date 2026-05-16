#pragma once

#include <string>
#include <string_view>

namespace util {

auto url_encode(std::string_view raw) -> std::string;

auto url_decode(std::string_view encoded) -> std::string;

} // namespace util
