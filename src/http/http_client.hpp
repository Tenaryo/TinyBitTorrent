#pragma once

#include <string>
#include <string_view>

namespace http {

auto get(std::string_view url) -> std::string;

} // namespace http
