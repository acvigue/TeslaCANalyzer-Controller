#ifndef DEBUG_HPP_
#define DEBUG_HPP_
#include <iomanip>
#include <iostream>
#include <sstream>

inline std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (int i = 0; i < s; i++) {
        ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
    }

    return ss.str();
}
#endif