#include "cmd/cmd.hpp"

#include <iostream>

namespace cmd {

auto dispatch(std::string_view command,
              std::span<const Command> commands,
              int argc,
              char** argv) -> int {
    for (const auto& cmd : commands) {
        if (command == cmd.name_) {
            if (argc < cmd.min_argc_) {
                std::cerr << "Usage: " << argv[0] << ' ' << cmd.usage_ << '\n';
                return 1;
            }
            return cmd.handler_(argc, argv);
        }
    }
    std::cerr << "unknown command: " << command << '\n';
    return 1;
}

} // namespace cmd
