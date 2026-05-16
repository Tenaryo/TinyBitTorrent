#pragma once

#include <string_view>

namespace cmd {

auto dispatch(std::string_view command, int argc, char** argv) -> int;

} // namespace cmd
