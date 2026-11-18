#include "cli_args.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <string_view>

namespace {

constexpr std::string_view input_header = "u_left,v_left,u_right,v_right";

std::string_view trim(std::string_view value) {
    constexpr std::string_view whitespace = " \t\r";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}

std::array<double, 4> parse_pixels(std::string_view row) {
    std::array<double, 4> values{};
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto comma = row.find(',');
        const bool last_field = i == values.size() - 1;
        if ((comma == std::string_view::npos) != last_field) {
            throw std::invalid_argument("expected four columns");
        }
        values[i] = stereo_cli::parse_number(std::string(trim(row.substr(0, comma))));
        if (!std::isfinite(values[i])) {
            throw std::invalid_argument("pixel coordinates must be finite");
        }
        if (!last_field) {
            row.remove_prefix(comma + 1);
        }
    }
    return values;
}

void write_row(const stereo_depth::Rig& rig, std::size_t number, std::string_view row,
               double max_vertical_mismatch) {
    std::cout << number << ',';
    try {
        const auto values = parse_pixels(row);
        const stereo_depth::Pixel left{values[0], values[1]};
        const stereo_depth::Pixel right{values[2], values[3]};
        if (left.u <= right.u) {
            std::cout << ",,,,,invalid_disparity\n";
            return;
        }
        const auto result = stereo_depth::triangulate(rig, left, right);
        if (result.vertical_mismatch_px > max_vertical_mismatch) {
            std::cout << ",,," << result.disparity_px << ',' << result.vertical_mismatch_px
                      << ",vertical_mismatch\n";
            return;
        }
        std::cout << result.point_m.x << ',' << result.point_m.y << ',' << result.point_m.z
                  << ',' << result.disparity_px << ',' << result.vertical_mismatch_px << ",ok\n";
    } catch (const std::logic_error&) {
        std::cout << ",,,,,malformed_row\n";
    } catch (const std::range_error&) {
        std::cout << ",,,,,numeric_range\n";
    }
}

void print_usage(std::ostream& stream) {
    stream << "Usage: stereo_batch fx fy cx cy baseline_m input.csv [--max-vertical-mismatch PX]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(std::cout);
        return 0;
    }
    if (argc != 7 && (argc != 9 || std::string_view(argv[7]) != "--max-vertical-mismatch")) {
        print_usage(std::cerr);
        return 2;
    }

    try {
        const auto rig = stereo_cli::parse_rig(argv + 1);
        stereo_depth::validate_rig(rig);
        double max_vertical_mismatch = std::numeric_limits<double>::infinity();
        if (argc == 9) {
            max_vertical_mismatch = stereo_cli::parse_number(argv[8]);
            if (!std::isfinite(max_vertical_mismatch) || max_vertical_mismatch < 0.0) {
                throw std::invalid_argument("maximum vertical mismatch must be finite and nonnegative");
            }
        }
        std::ifstream input(argv[6]);
        if (!input) {
            throw std::runtime_error("could not open input file");
        }

        std::string line;
        if (!std::getline(input, line)) {
            throw std::runtime_error("could not read input header");
        }
        std::string_view header(line);
        constexpr std::string_view utf8_bom = "\xEF\xBB\xBF";
        if (header.substr(0, utf8_bom.size()) == utf8_bom) {
            header.remove_prefix(utf8_bom.size());
        }
        if (trim(header) != input_header) {
            throw std::invalid_argument("expected header u_left,v_left,u_right,v_right");
        }

        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(12)
                  << "row,x_m,y_m,z_m,disparity_px,vertical_mismatch_px,status\n";
        std::size_t number = 0;
        while (std::getline(input, line)) {
            write_row(rig, ++number, line, max_vertical_mismatch);
        }
        if (!input.eof() || input.bad()) {
            throw std::runtime_error("could not finish reading input file");
        }
        if (!std::cout.flush()) {
            throw std::runtime_error("could not write output CSV");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "stereo_batch: " << error.what() << '\n';
        return 1;
    }
}
