#include "stereo_depth/stereo.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>

namespace {

stereo_depth::Pixel project(const stereo_depth::Rig& rig,
                            const stereo_depth::Point3& point_m, double camera_x_m) {
    return {rig.cx + rig.fx * (point_m.x - camera_x_m) / point_m.z,
            rig.cy + rig.fy * point_m.y / point_m.z};
}

}  // namespace

int main() {
    constexpr stereo_depth::Rig rig{768, 768, 320, 240, 0.125};
    constexpr double step_m = 0.125;
    constexpr int half_width = 4;
    constexpr int half_height = 3;
    constexpr int near_depth = 16;
    constexpr int far_depth = 24;

    std::cout.imbue(std::locale::classic());
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "u_left,v_left,u_right,v_right\n";
    for (int z = near_depth; z <= far_depth; ++z) {
        for (int y = -half_height; y <= half_height; ++y) {
            for (int x = -half_width; x <= half_width; ++x) {
                const bool on_x_face = std::abs(x) == half_width;
                const bool on_y_face = std::abs(y) == half_height;
                const bool on_z_face = z == near_depth || z == far_depth;
                // An edge lies on two or more faces. Visit each corner once.
                if (on_x_face + on_y_face + on_z_face < 2) {
                    continue;
                }
                const stereo_depth::Point3 point{x * step_m, y * step_m, z * step_m};
                const auto left = project(rig, point, 0.0);
                const auto right = project(rig, point, rig.baseline_m);
                std::cout << left.u << ',' << left.v << ',' << right.u << ',' << right.v << '\n';
            }
        }
    }
    return std::cout.flush() ? 0 : 1;
}
