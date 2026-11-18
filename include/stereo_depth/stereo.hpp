#pragma once

namespace stereo_depth {

struct Pixel {
    double u;
    double v;
};

struct Point3 {
    double x;
    double y;
    double z;
};

// Both rectified cameras use the same intrinsics. The right camera center
// is at (+baseline_m, 0, 0) in the left camera frame.
struct Rig {
    double fx;
    double fy;
    double cx;
    double cy;
    double baseline_m;
};

struct Reconstruction {
    Point3 point_m;
    double disparity_px;
    double vertical_mismatch_px;
};

// Throws invalid_argument if the camera model is invalid.
void validate_rig(const Rig& rig);

// Throws invalid_argument for invalid calibration or observations and
// range_error when the result falls outside the representable numeric range.
Reconstruction triangulate(const Rig& rig, Pixel left, Pixel right);

}  // namespace stereo_depth
