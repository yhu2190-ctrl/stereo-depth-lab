#pragma once

#include "stereo_depth/stereo.hpp"

#include <stdexcept>
#include <string>

namespace stereo_cli {

inline double parse_number(const std::string& text) {
    std::size_t consumed = 0;
    const double value = std::stod(text, &consumed);
    if (consumed != text.size()) {
        throw std::invalid_argument("expected a number, got " + text);
    }
    return value;
}

inline stereo_depth::Rig parse_rig(char* const* args) {
    return {parse_number(args[0]), parse_number(args[1]), parse_number(args[2]),
            parse_number(args[3]), parse_number(args[4])};
}

}  // namespace stereo_cli
