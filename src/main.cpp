#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

#include "bencode/decoder.hpp"
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

namespace {

auto to_json(const bencode::Value& value) -> json {
    return std::visit(
        [](const auto& val) -> json {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bencode::List>) {
                json arr = json::array();
                for (const auto& elem : val.elements_) {
                    arr.push_back(to_json(elem));
                }
                return arr;
            } else {
                return json(val);
            }
        },
        value);
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
