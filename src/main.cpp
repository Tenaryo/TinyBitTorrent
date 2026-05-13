#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

auto decode_bencoded_value(const std::string& encoded_value) -> json {
    if (std::isdigit(encoded_value[0]) != 0) {
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::atoll(number_string.c_str());
            auto length = static_cast<size_t>(number);
            std::string str = encoded_value.substr(colon_index + 1, length);
            return {str};
        }
        throw std::runtime_error("Invalid encoded value: " + encoded_value);
    }
    throw std::runtime_error("Unhandled encoded value: " + encoded_value);
}

auto main(int argc, char* argv[]) -> int {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>"
                      << '\n';
            return 1;
        }

        std::string command = argv[1];

        if (command == "decode") {
            if (argc < 3) {
                std::cerr << "Usage: " << argv[0] << " decode <encoded_value>"
                          << '\n';
                return 1;
            }

            std::string encoded_value = argv[2];
            json decoded_value = decode_bencoded_value(encoded_value);
            std::cout << decoded_value.dump() << '\n';
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
