# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains C++ programs for working with ArUco augmented reality markers using OpenCV 4. The programs handle marker generation, detection, camera calibration, and pose estimation.

**Important**: Requires OpenCV 4 (for OpenCV 3, use the `cv3` branch).

## Build Commands

Each program is built independently using CMake. The pattern is the same for all:

```bash
cd <program_directory>
mkdir build && cd build
cmake ../
make
```

Programs:
- `create_markers/` - Generates marker images and boards
- `detect_marker/` - Detects markers from camera/video
- `camera_calibration/` - Calibrates camera using ArUco board
- `pose_estimation/` - Estimates marker pose (translation/rotation)
- `draw_cube/` - Draws 3D cube overlay on detected markers

## Running Programs

**Generate markers:**
```bash
./generate_marker --b=1 -d=16 --id=108 --ms=400 --si marker.jpg
./generate_board --bb=1 -h=2 -w=4 -l=200 -s=100 -d=16 --si board.jpg
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

### OpenCV ArUco API Usage
All programs use the OpenCV ArUco module (`opencv2/aruco.hpp`):
- `cv::aruco::detectMarkers()` - Marker detection
- `cv::aruco::estimatePoseSingleMarkers()` - Pose estimation
- `cv::aruco::drawDetectedMarkers()` - Visualization
- `cv::aruco::drawAxis()` - Draw coordinate axes

## Docker Support

For non-Ubuntu systems or isolated builds:
```bash
bash docker_build.sh       # Build image
xhost +                    # Enable GUI (Linux)
bash docker_start.sh       # Start container
bash docker_opencv_setup.sh # Install OpenCV (first time only)
```

Or pull pre-built: `docker pull kanishgama/aruco-markers:opencv-4.5.3`
