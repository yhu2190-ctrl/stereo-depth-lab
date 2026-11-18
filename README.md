# Stereo Depth Lab

A dependency-free C++17 tool for reconstructing 3D points from matched pixels in a rectified stereo pair. It reports the reconstructed point, horizontal disparity, and vertical mismatch.

一个用 C++17 写的双目三角化工具。给定已校正的左右图像中同一个点的像素坐标，以及相机内参和基线距离，计算该点在左相机坐标系下的三维位置。

## Build and run

Install a C++17 compiler and CMake 3.20 or newer. On Windows, you can run these commands from a Visual Studio Developer PowerShell with CMake available.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

```powershell
# Windows with a Visual Studio generator
.\build\Release\stereo_point.exe 800 800 320 240 0.12 400 260 376 260
```

```sh
# Linux or macOS with a single-config generator
./build/stereo_point 800 800 320 240 0.12 400 260 376 260
```

```text
x_m=0.4 y_m=0.1 z_m=4
disparity_px=24 vertical_mismatch_px=0
```

参数依次为 `fx fy cx cy baseline_m u_left v_left u_right v_right`。焦距和像素坐标以像素为单位，基线和三维结果以米为单位。左相机的 X 轴向右、Y 轴向下、Z 轴向前。

## How it works · 公式怎么来的

两台校正后的相机使用相同内参，右相机相对左相机沿 X 轴向右平移了距离 B。空间点在右相机坐标系下的 X 坐标因此为 X − B。针孔投影给出：

```text
u_left  = fx × X / Z + cx
u_right = fx × (X − B) / Z + cx
d       = u_left − u_right = fx × B / Z
```

所以深度 Z = fx × B / d。示例里 B = 0.12 m，d = 24 px，手算得到 Z = 800 × 0.12 / 24 = 4 m。再用左图的 u 恢复 X，用两张图的 v 的平均值恢复 Y。理想校正且匹配准确时，两张图的 v 相同，工具也会单独报告它们的差值绝对值。

现有测试用已知三维点生成左右像素，再检查三角化能否恢复原坐标。工具会拒绝非正视差。很小的正视差仍可计算，但深度对像素误差更敏感。

## Scope · 当前范围

v0.1 接受一对已匹配的像素坐标。相机标定、图像校正和特征匹配由上游完成。下一步计划做可复现的像素噪声实验，测量不同深度的误差，再考虑批量读取真实匹配点。

See [Hartley and Sturm, *Triangulation*](https://doi.org/10.1006/cviu.1997.0547) for triangulation with general camera poses.

MIT licensed. See [LICENSE](LICENSE).
