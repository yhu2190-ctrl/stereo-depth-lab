#include "stereo_depth/stereo.hpp"

#include <array>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <locale>

int main() {
    const stereo_depth::Rig rig{800, 800, 320, 240, 0.12};
    const std::array<double, 7> true_depths{2, 4, 8, 12, 24, 48, 96};
    const stereo_depth::Pixel left{rig.cx, rig.cy};

    std::cout.imbue(std::locale::classic());
    std::cout << std::setprecision(10)
              << "true_depth_m,true_disparity_px,disparity_offset_px,observed_disparity_px,"
                 "case_count,estimated_depth_m,signed_depth_error_m,linearized_depth_error_m,status\n";

    for (const double true_depth : true_depths) {
        // The synthetic point is on the left camera's optical axis.
        const double true_disparity = rig.fx * rig.baseline_m / true_depth;

        // Independent offsets of -1, 0, or +1 in each image produce five
        // possible disparity offsets. Only their difference affects depth.
        for (int offset = -2; offset <= 2; ++offset) {
            const int case_count = 3 - std::abs(offset);
            const stereo_depth::Pixel right{rig.cx - true_disparity - offset, rig.cy};
            const double observed_disparity = left.u - right.u;

            std::cout << true_depth << ',' << true_disparity << ',' << offset << ','
                      << observed_disparity << ',' << case_count << ',';
            if (observed_disparity <= 0.0) {
                std::cout << ",,,invalid_disparity\n";
                continue;
            }

            const auto result = stereo_depth::triangulate(rig, left, right);
            const double estimated_depth = result.point_m.z;
            const double linearized_error = offset == 0 ? 0.0 : -true_depth * offset / true_disparity;
            std::cout << estimated_depth << ',' << estimated_depth - true_depth << ','
                      << linearized_error << ",ok\n";
        }
    }
    return std::cout ? 0 : 1;
}
