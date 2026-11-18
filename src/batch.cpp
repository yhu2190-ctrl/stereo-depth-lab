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
#include <vector>

namespace {

constexpr std::string_view input_header = "u_left,v_left,u_right,v_right";

struct Options {
    double max_vertical_mismatch = std::numeric_limits<double>::infinity();
    bool export_ply = false;
};

struct BatchRow {
    std::string_view status;
    stereo_depth::Reconstruction reconstruction{};
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 7; i < argc; i += 2) {
        const std::string_view name(argv[i]);
        const std::string_view value(argv[i + 1]);
        if (name == "--format") {
            if (value != "csv" && value != "ply") {
                throw std::invalid_argument("output format must be csv or ply");
            }
            options.export_ply = value == "ply";
        } else if (name == "--max-vertical-mismatch") {
            const double threshold = stereo_cli::parse_number(argv[i + 1]);
            if (!std::isfinite(threshold) || threshold < 0.0) {
                throw std::invalid_argument("maximum vertical mismatch must be finite and nonnegative");
            }
            options.max_vertical_mismatch = threshold;
        } else {
            throw std::invalid_argument("unrecognized option " + std::string(name));
        }
    }
    return options;
}

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

BatchRow reconstruct_row(const stereo_depth::Rig& rig, std::string_view row,
                         double max_vertical_mismatch) {
    try {
        const auto values = parse_pixels(row);
        const stereo_depth::Pixel left{values[0], values[1]};
        const stereo_depth::Pixel right{values[2], values[3]};
        if (left.u <= right.u) {
            return {"invalid_disparity"};
        }
        const auto result = stereo_depth::triangulate(rig, left, right);
        if (result.vertical_mismatch_px > max_vertical_mismatch) {
            return {"vertical_mismatch", result};
        }
        return {"ok", result};
    } catch (const std::logic_error&) {
        return {"malformed_row"};
    } catch (const std::range_error&) {
        return {"numeric_range"};
    }
}

void write_csv_row(std::size_t number, const BatchRow& row) {
    const auto& result = row.reconstruction;
    std::cout << number << ',';
    if (row.status == "ok") {
        std::cout << result.point_m.x << ',' << result.point_m.y << ',' << result.point_m.z;
    } else {
        std::cout << ",,";
    }
    std::cout << ',';
    if (row.status == "ok" || row.status == "vertical_mismatch") {
        std::cout << result.disparity_px << ',' << result.vertical_mismatch_px;
    } else {
        std::cout << ',';
    }
    std::cout << ',' << row.status << '\n';
}

void write_ply(const std::vector<stereo_depth::Point3>& points) {
    std::cout << "ply\n"
              << "format ascii 1.0\n"
              << "comment Stereo Depth Lab - left camera coordinates in meters\n"
              << "element vertex " << points.size() << '\n'
              << "property double x\n"
              << "property double y\n"
              << "property double z\n"
              << "end_header\n";
    for (const auto& point : points) {
        std::cout << point.x << ' ' << point.y << ' ' << point.z << '\n';
    }
}

void print_usage(std::ostream& stream) {
    stream << "Usage: stereo_batch fx fy cx cy baseline_m input.csv "
              "[--max-vertical-mismatch PX] [--format csv|ply]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(std::cout);
        return 0;
    }
    if (argc < 7 || (argc - 7) % 2 != 0) {
        print_usage(std::cerr);
        return 2;
    }

    try {
        const auto rig = stereo_cli::parse_rig(argv + 1);
        stereo_depth::validate_rig(rig);
        const auto options = parse_options(argc, argv);
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
        std::cout << std::setprecision(12);
        if (!options.export_ply) {
            std::cout << "row,x_m,y_m,z_m,disparity_px,vertical_mismatch_px,status\n";
        }

        std::vector<stereo_depth::Point3> points;
        std::size_t number = 0;
        while (std::getline(input, line)) {
            ++number;
            const auto row = reconstruct_row(rig, line, options.max_vertical_mismatch);
            if (!options.export_ply) {
                write_csv_row(number, row);
            } else if (row.status == "ok") {
                points.push_back(row.reconstruction.point_m);
            }
        }
        if (!input.eof() || input.bad()) {
            throw std::runtime_error("could not finish reading input file");
        }
        if (options.export_ply) {
            write_ply(points);
        }
        if (!std::cout.flush()) {
            throw std::runtime_error("could not write output");
        }
        if (options.export_ply) {
            std::cerr << "stereo_batch: exported " << points.size() << " of " << number << " rows\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "stereo_batch: " << error.what() << '\n';
        return 1;
    }
}
