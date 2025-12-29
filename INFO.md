# ArUco Markers GUI Application

A Windows desktop application providing a graphical interface for ArUco and ChArUco marker operations using Dear ImGui and DirectX 12.

## Overview

The `aruco_gui` application integrates all ArUco marker tools into a single tabbed interface, eliminating the need for command-line interaction. It supports marker generation, board creation (ArUco and ChArUco), real-time detection, camera calibration, pose estimation, and 3D cube rendering.

## Requirements

- Windows 10/11
- Visual Studio 2022
- vcpkg with dependencies:
  - `opencv4[contrib]:x64-windows`
  - `imgui[dx12-binding,win32-binding]:x64-windows`

## Building

```powershell
# Set vcpkg root
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Configure and build
cmake --preset windows-x64-release
cmake --build build --config Release

# Executable location
build\gui_app\Release\aruco_gui.exe
```

Or open `build\aruco_markers.sln` in Visual Studio (aruco_gui is set as startup project).

## Running

Simply launch the executable - no command-line arguments needed:

```powershell
.\build\gui_app\Release\aruco_gui.exe
```

Test data and calibration files are automatically copied to the output directory during build.

## Features

### Marker Panel
Generate individual ArUco markers with configurable:
- Dictionary type (all OpenCV ArUco dictionaries)
- Marker ID (0-1000)
- Size in pixels (50-2000)
- Border bits (1-5)
- Live preview and save to PNG/JPEG

### Board Panel
Create grid boards of ArUco or ChArUco markers with two modes:

**Custom Board Tab:**
- Configurable grid dimensions (W x H)
- Marker length and separation in pixels
- Margin settings
- Auto-calculated output dimensions
- Live preview and export

**Calibration Board Tab:**
- **Board type selection:** ArUco Grid or ChArUco
- Paper format selection:
  - A4 (210 x 297 mm)
  - A3 (297 x 420 mm)
  - A2 (420 x 594 mm)
  - ANSI A / Letter (8.5 x 11 in)
  - ANSI B (11 x 17 in)
  - ANSI C (17 x 22 in)
- Grid size configuration (markers for ArUco, squares for ChArUco)
- Print DPI (72-600, default 300)
- Page margin in mm
- Marker ratio slider (controls marker vs. gap/square size)
- Auto-calculated physical dimensions (mm)
- Displays exact parameters for the Calibration/Pose tools
- "Copy Parameters" button for clipboard
- Generates print-ready images at correct resolution

**ChArUco Boards:**
ChArUco boards combine a chessboard pattern with ArUco markers, providing more accurate corner detection for calibration and pose estimation. The chessboard corners enable sub-pixel accuracy, while the ArUco markers allow for partial board visibility and unique identification.

### Detect Panel
Real-time marker detection:
- Video source: camera, video file, or test video
- Live feed with detection overlay
- Marker count display
- FPS monitoring

### Calibration Panel
Camera calibration using ArUco or ChArUco boards:
- **Board type selection:** ArUco Grid or ChArUco
- Board configuration:
  - For ArUco: grid size, marker length, separation (meters)
  - For ChArUco: squares X/Y, square length, marker length (meters)
- Calibration options (refine detection, zero tangent distortion, fix principal point, fix aspect ratio)
- Frame capture interface with visual feedback
- Reprojection error display
- Export to YAML calibration file

**ChArUco Calibration Benefits:**
- More accurate corner detection (sub-pixel precision)
- Works with partially visible boards
- Better results with fewer calibration frames
- Robust to occlusion

### Pose Panel
3D pose estimation of detected markers or ChArUco boards:
- **Board type selection:** ArUco (individual markers) or ChArUco (board pose)
- For ArUco: marker length configuration (meters)
- For ChArUco: board parameters (squares X/Y, square length, marker length)
- Calibration file loading
- Real-time translation and rotation vectors
- ChArUco mode shows board pose at top-left corner origin

**ChArUco Pose Estimation:**
When using ChArUco mode, the pose is estimated for the entire board (at the top-left corner), providing more stable and accurate pose estimation than individual markers, especially when multiple corners are visible.

### Cube Panel
3D cube overlay rendering:
- All pose estimation features
- Video recording capability (AVI format)
- Frame counter for recordings

## Interface

**Navigation:** Use the View menu or tabs to switch between panels.

**Video Sources:**
- **Camera:** Select from available cameras
- **Video File:** Browse for video files
- **Test Video:** Uses bundled `test_data/test_video.mp4`

**File Operations:**
- Use Browse buttons to select input/output files
- Calibration files use YAML format (.yml)
- Images save as PNG or JPEG

## Tips

1. **First-time setup:** Run camera calibration before using Pose or Cube panels
2. **Calibration workflow:** Use Board Panel's "Calibration Board" tab to generate a print-ready board, then use the displayed parameters in the Calibration panel
3. **ChArUco recommended:** For best calibration accuracy, use ChArUco boards instead of plain ArUco grids
4. **Default calibration:** Enable "Use default calibration" to use `calibration_params.yml`
5. **Calibration quality:** Capture 10-20 frames at various angles for best results
6. **Marker length:** Must match physical marker size for accurate pose estimation
7. **ChArUco constraint:** Marker length must be smaller than square length (typically 80% of square size)
8. **Test video:** Good for verifying setup without a camera

## Architecture

```
gui_app/
├── src/
│   ├── main.cpp              # Application entry, DX12 window setup
│   ├── dx12_backend.cpp      # DirectX 12 device and swap chain management
│   ├── texture_manager.cpp   # OpenCV Mat to DX12 texture conversion
│   ├── video_thread.cpp      # Background video capture thread
│   └── panels/               # Feature panels
│       ├── panel_base.cpp    # Common UI helpers
│       ├── marker_panel.cpp
│       ├── board_panel.cpp
│       ├── detect_panel.cpp
│       ├── calibration_panel.cpp
│       ├── pose_panel.cpp
│       └── cube_panel.cpp
└── include/gui/              # Headers
    ├── dx12_backend.hpp
    ├── texture_manager.hpp
    ├── video_thread.hpp
    └── panels/
```

All panels use the shared `aruco_lib` library for ArUco/ChArUco operations.

### aruco_lib Library

The `aruco_lib` library provides core functionality:

```
aruco_lib/
├── include/aruco_lib/
│   ├── types.hpp             # Data structures (BoardType, params, results)
│   ├── board_generator.hpp   # ArUco and ChArUco board generation
│   ├── camera_calibrator.hpp # Calibration with ArUco/ChArUco support
│   ├── pose_estimator.hpp    # Pose estimation for markers and boards
│   └── ...
└── src/
    └── ...
```

**Key Types:**
- `BoardType`: Enum for ArUco vs ChArUco selection
- `CalibrationBoardParams`: Print-ready board generation parameters
- `CalibrationParams`: Camera calibration configuration
- `PoseParams`: Pose estimation configuration
- `DetectionResult`: Detection output with optional board pose

## CLI Tool: generate_calibration_board

A command-line tool is also available for generating print-ready calibration boards:

```powershell
# Generate A4 calibration board
generate_calibration_board board_a4.png -f=A4 -w=5 -h=7 -d=16

# Generate Letter calibration board
generate_calibration_board board_letter.png -f=Letter -w=5 -h=6 -d=16
```

**Options:**
| Flag | Description | Default |
|------|-------------|---------|
| `-f, --format` | Paper format: A4, A3, A2, Letter, ANSI_B, ANSI_C | required |
| `-w` | Markers/Squares in X direction | required |
| `-h` | Markers/Squares in Y direction | required |
| `-d` | Dictionary ID | 16 |
| `--dpi` | Print resolution | 300 |
| `--margin` | Page margin in mm | 10 |
| `--ratio` | Marker to cell/square size ratio | 0.8 |
| `--si` | Show generated image | false |

The tool outputs the physical dimensions and a ready-to-use `camera_calibration` command with all parameters.

## Troubleshooting

**Black video preview:** Ensure camera is not in use by another application.

**Pose estimation unavailable:** Load a calibration file first (button turns green when loaded).

**Poor detection:** Adjust lighting or try a different ArUco dictionary matching your markers.

**ChArUco crash:** Ensure marker length is smaller than square length. The app validates this automatically, but manual parameter entry should respect this constraint.

**Build errors:** Verify vcpkg packages are installed with correct features:
```powershell
vcpkg list
# Should show: opencv4, imgui with dx12-binding and win32-binding
```

**DX12 issues:** Ensure your GPU supports DirectX 12. The application requires Windows 10 or later with a DX12-capable graphics adapter.
