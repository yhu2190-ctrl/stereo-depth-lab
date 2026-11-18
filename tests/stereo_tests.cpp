#include "stereo_depth/stereo.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using stereo_depth::Pixel;
using stereo_depth::Point3;
using stereo_depth::Rig;
using stereo_depth::triangulate;

namespace {

constexpr Rig rig{800, 720, 320, 240, 0.12};

void expect_near(double actual, double expected, const char* label) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > 1e-10) {
        throw std::runtime_error(std::string(label) + ": unexpected value " + std::to_string(actual));
    }
}

template<class Error>
void expect_rejection(const Rig& cameras, Pixel left, Pixel right, const char* label) {
    try {
        triangulate(cameras, left, right);
    } catch (const Error&) {
        return;
    }
    throw std::runtime_error(std::string(label) + ": expected triangulation to reject the input");
}

void reconstruct_known_points() {
    const Point3 points[]{{0.4, 0.1, 4.0}, {-0.3, -0.4, 2.0}, {0.0, 0.0, 12.0}};
    for (const auto& point : points) {
        const double v = rig.cy + rig.fy * point.y / point.z;
        const Pixel left{rig.cx + rig.fx * point.x / point.z, v};
        const Pixel right{rig.cx + rig.fx * (point.x - rig.baseline_m) / point.z, v};
        const auto result = triangulate(rig, left, right);
        expect_near(result.point_m.x, point.x, "x");
        expect_near(result.point_m.y, point.y, "y");
        expect_near(result.point_m.z, point.z, "z");
        expect_near(result.disparity_px, rig.fx * rig.baseline_m / point.z, "disparity");
        expect_near(result.vertical_mismatch_px, 0.0, "vertical mismatch");
    }
}

void report_vertical_mismatch() {
    const auto result = triangulate(rig, {400, 256}, {376, 260});
    expect_near(result.point_m.y, 0.1, "y from mean v");
    expect_near(result.vertical_mismatch_px, 4.0, "vertical mismatch");
}

void reject_invalid_inputs() {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto largest = std::numeric_limits<double>::max();
    expect_rejection<std::invalid_argument>(rig, {400, 260}, {400, 260}, "zero disparity");
    expect_rejection<std::invalid_argument>(rig, {400, 260}, {401, 260}, "negative disparity");
    expect_rejection<std::invalid_argument>(rig, {400, nan}, {376, 260}, "non-finite pixel");
    expect_rejection<std::invalid_argument>({800, 720, 320, 240, 0}, {400, 260}, {376, 260}, "zero baseline");
    expect_rejection<std::invalid_argument>({800, 0, 320, 240, 0.12}, {400, 260}, {376, 260}, "zero focal length");
    expect_rejection<std::range_error>(rig, {largest, 260}, {-largest, 260}, "overflowing disparity");
}

}  // namespace

int main() {
    try {
        reconstruct_known_points();
        report_vertical_mismatch();
        reject_invalid_inputs();
        std::cout << "All stereo triangulation checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
