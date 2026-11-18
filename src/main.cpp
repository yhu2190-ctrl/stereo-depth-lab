#include "stereo_depth/stereo.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

double parse_number(const char* argument) {
    const std::string text(argument);
    std::size_t consumed = 0;
    const double value = std::stod(text, &consumed);
    if (consumed != text.size()) {
        throw std::invalid_argument("expected a number, got " + text);
    }
    return value;
}

void print_usage(std::ostream& stream) {
    stream << "Usage: stereo_point fx fy cx cy baseline_m u_left v_left u_right v_right\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(std::cout);
        return 0;
    }
    if (argc != 10) {
        print_usage(std::cerr);
        return 2;
    }

    try {
        const stereo_depth::Rig rig{
            parse_number(argv[1]), parse_number(argv[2]), parse_number(argv[3]),
            parse_number(argv[4]), parse_number(argv[5])};
        const stereo_depth::Pixel left{parse_number(argv[6]), parse_number(argv[7])};
        const stereo_depth::Pixel right{parse_number(argv[8]), parse_number(argv[9])};
        const auto result = stereo_depth::triangulate(rig, left, right);
        const auto& point = result.point_m;

        std::cout << std::setprecision(12)
                  << "x_m=" << point.x << " y_m=" << point.y << " z_m=" << point.z << '\n'
                  << "disparity_px=" << result.disparity_px
                  << " vertical_mismatch_px=" << result.vertical_mismatch_px << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "stereo_point: " << error.what() << '\n';
        return 1;
    }
}
