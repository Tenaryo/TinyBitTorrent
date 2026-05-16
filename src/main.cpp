#include <iostream>
#include <stdexcept>

#include "cmd/cmd.hpp"

auto main(int argc, char* argv[]) -> int {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <command> [args]\n";
            return 1;
        }
        return cmd::dispatch(argv[1], argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
