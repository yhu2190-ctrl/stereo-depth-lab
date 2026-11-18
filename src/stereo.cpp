#include "stereo_depth/stereo.hpp"

#include <cmath>
#include <stdexcept>

namespace stereo_depth {

void validate_rig(const Rig& rig) {
    if (!std::isfinite(rig.fx) || !std::isfinite(rig.fy) ||
        !std::isfinite(rig.cx) || !std::isfinite(rig.cy) ||
        !std::isfinite(rig.baseline_m) ||
        rig.fx <= 0.0 || rig.fy <= 0.0 || rig.baseline_m <= 0.0) {
        throw std::invalid_argument("focal lengths and baseline must be positive, and all rig values must be finite");
    }
}

Reconstruction triangulate(const Rig& rig, Pixel left, Pixel right) {
    validate_rig(rig);
    if (!std::isfinite(left.u) || !std::isfinite(left.v) ||
        !std::isfinite(right.u) || !std::isfinite(right.v)) {
        throw std::invalid_argument("pixel coordinates must be finite");
    }

    const double disparity = left.u - right.u;
    if (disparity <= 0.0) {
        throw std::invalid_argument("left u must be greater than right u for a point at positive depth");
    }

    const double mean_v = 0.5 * left.v + 0.5 * right.v;
    const double normalized_x = (left.u - rig.cx) / rig.fx;
    const double normalized_y = (mean_v - rig.cy) / rig.fy;
    const double z = (rig.fx / disparity) * rig.baseline_m;
    const Point3 point{normalized_x * z, normalized_y * z, z};
    const double mismatch = std::abs(left.v - right.v);
    if (!std::isfinite(disparity) || !std::isfinite(mismatch) ||
        !std::isfinite(point.x) || !std::isfinite(point.y) ||
        !std::isfinite(point.z) || point.z <= 0.0) {
        throw std::range_error("reconstruction is outside the floating-point range");
    }

    return {point, disparity, mismatch};
}

}  // namespace stereo_depth
