# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains C++ programs for working with ArUco augmented reality markers using OpenCV 4. The programs handle marker generation, detection, camera calibration, and pose estimation.

**Important**: Requires OpenCV 4.7+ (uses the new ArUco API in `opencv2/objdetect`).

## Build Commands

### Windows (Visual Studio 2022 + vcpkg)

**Prerequisites:**
- Visual Studio 2022 with C++ workload
- vcpkg with OpenCV installed: `vcpkg install opencv4[contrib]:x64-windows`

**Build from root directory (unified solution):**
```powershell
# Configure with vcpkg toolchain
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build all projects
cmake --build build --config Release

# Or open in Visual Studio
start build/aruco_markers.sln
```

**Using CMake Presets (requires VCPKG_ROOT environment variable):**
```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset windows-x64-release
cmake --build build --config Release
```

### Linux/macOS

```bash
mkdir build && cd build
cmake ..
make
```

Or build individual programs:
```bash
cd <program_directory>
mkdir build && cd build
cmake ../
make
```

## Programs

- `create_markers/` - Generates marker images and boards
  - `generate_marker` - Creates single marker images
  - `generate_board` - Creates grid board images
- `detect_marker/` - Detects markers from camera/video
- `camera_calibration/` - Calibrates camera using ArUco board
- `pose_estimation/` - Estimates marker pose (translation/rotation)
- `draw_cube/` - Draws 3D cube overlay on detected markers

## Running Programs

**Generate markers:**
```bash
./generate_marker -d=16 --id=108 --ms=400 --si marker.jpg
./generate_board -w=4 -h=2 -l=200 -s=100 -d=16 --si board.jpg
```

**Detect markers (with test video):**
```bash
./detect_markers -v=../../test_data/test_video.mp4
```

**Camera calibration:**
```bash
./camera_calibration -d=16 -dp=../detector_params.yml -h=2 -w=4 -l=<marker_length_meters> -s=<separation_meters> ../../calibration_params.yml
```

**Pose estimation / Draw cube:**
```bash
./pose_estimation -l=<marker_length_meters> -v=../../test_data/test_video.mp4
./draw_cube -l=<marker_length_meters> -v=../../test_data/test_video.mp4
```

## Architecture

### Shared Code
- `common/include/fdcl_common.hpp` - Shared utilities including:
  - `fdcl::keys` - Common command-line argument definitions
  - `parse_inputs()` - Command-line parser setup
  - `parse_video_in()` - Video source initialization (camera or file)
  - `open_video_from_arg()` - Opens video by ID or URL
  - `drawText()` - Overlay text on frames

### Key Dependencies
- Camera calibration params stored in `calibration_params.yml` at repo root
- Programs expect calibration file at `../../calibration_params.yml` relative to build directory
- Dictionary ID 16 (`DICT_ARUCO_ORIGINAL`) is the default

### OpenCV ArUco API (4.7+)
All programs use the OpenCV ArUco module from `opencv2/objdetect`:
- `cv::aruco::ArucoDetector` - Detector class with `detectMarkers()` method
- `cv::aruco::Dictionary` - Marker dictionary (use `getPredefinedDictionary()`)
- `cv::aruco::GridBoard` - Board class with `generateImage()` and `matchImagePoints()`
- `cv::aruco::generateImageMarker()` - Generate marker images
- `cv::aruco::drawDetectedMarkers()` - Visualization
- `cv::solvePnP()` + `cv::drawFrameAxes()` - Pose estimation and axis drawing

### Build Configuration
- `CMakeLists.txt` (root) - Unified solution for all programs
- `CMakePresets.json` - VS2022 + vcpkg configuration presets
- Each subdirectory has its own `CMakeLists.txt` for standalone builds

## Docker Support

For non-Ubuntu systems or isolated builds:
```bash
bash docker_build.sh       # Build image
xhost +                    # Enable GUI (Linux)
bash docker_start.sh       # Start container
bash docker_opencv_setup.sh # Install OpenCV (first time only)
```

Or pull pre-built: `docker pull kanishgama/aruco-markers:opencv-4.5.3`
