#include "cli_args.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {

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
        const auto rig = stereo_cli::parse_rig(argv + 1);
        const stereo_depth::Pixel left{stereo_cli::parse_number(argv[6]), stereo_cli::parse_number(argv[7])};
        const stereo_depth::Pixel right{stereo_cli::parse_number(argv[8]), stereo_cli::parse_number(argv[9])};
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
