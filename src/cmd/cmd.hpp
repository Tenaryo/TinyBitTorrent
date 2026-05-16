#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace cmd {

struct Command {
    std::string_view name_;
    std::string_view usage_;
    int min_argc_;
    int (*handler_)(int argc, char** argv);
};

auto dispatch(std::string_view command,
              std::span<const Command> commands,
              int argc,
              char** argv) -> int;

} // namespace cmd
