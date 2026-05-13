#include <iostream>
#include <string>
#include <variant>

#include "bencode/decoder.hpp"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

namespace {

auto to_json(const bencode::Value& value) -> json {
    return std::visit([](const auto& v) -> json { return json(v); }, value);
}

} // anonymous namespace

auto main(int argc, char* argv[]) -> int {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>\n";
            return 1;
        }

        std::string command = argv[1];

        if (command == "decode") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0]
                          << " decode <encoded_value>\n";
                return 1;
            }

            auto decoded = bencode::decode(argv[2]);
            std::cout << to_json(decoded).dump() << '\n';
        } else {
            std::cerr << "unknown command: " << command << '\n';
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
