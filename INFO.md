# ArUco Markers GUI Application

A Windows desktop application providing a graphical interface for ArUco marker operations using Dear ImGui and DirectX 11.

## Overview

The `aruco_gui` application integrates all ArUco marker tools into a single tabbed interface, eliminating the need for command-line interaction. It supports marker generation, board creation, real-time detection, camera calibration, pose estimation, and 3D cube rendering.

## Requirements

- Windows 10/11
- Visual Studio 2022
- vcpkg with dependencies:
  - `opencv4[contrib]:x64-windows`
  - `imgui[dx11-binding,win32-binding]:x64-windows`

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
Create grid boards of ArUco markers with two modes:

**Custom Board Tab:**
- Configurable grid dimensions (W x H)
- Marker length and separation in pixels
- Margin settings
- Auto-calculated output dimensions
- Live preview and export

**Calibration Board Tab:**
- Paper format selection (A4 or Letter)
- Grid size configuration
- Print DPI (72-600, default 300)
- Page margin in mm
- Marker ratio slider (controls marker vs. gap size)
- Auto-calculated physical dimensions (mm)
- Displays exact parameters for the Calibration tool
- "Copy Parameters" button for clipboard
- Generates print-ready images at correct resolution

The Calibration Board mode creates boards specifically designed for camera calibration. After printing, use the displayed parameters (marker length and separation in meters) directly in the Calibration panel.

### Detect Panel
Real-time marker detection:
- Video source: camera, video file, or test video
- Live feed with detection overlay
- Marker count display
- FPS monitoring

### Calibration Panel
Camera calibration using ArUco boards:
- Board configuration (grid size, marker dimensions in meters)
- Calibration options (refine detection, zero tangent distortion, fix principal point, fix aspect ratio)
- Frame capture interface
- Reprojection error display
- Export to YAML calibration file

### Pose Panel
3D pose estimation of detected markers:
- Marker length configuration (meters)
- Calibration file loading
- Real-time translation and rotation vectors
- Rotation matrix display

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
3. **Default calibration:** Enable "Use default calibration" to use `calibration_params.yml`
4. **Calibration quality:** Capture 10-20 frames at various angles for best results
5. **Marker length:** Must match physical marker size for accurate pose estimation
6. **Test video:** Good for verifying setup without a camera

## Architecture

```
gui_app/
├── src/
│   ├── main.cpp              # Application entry, window setup
│   ├── texture_manager.cpp   # OpenCV Mat to DX11 texture conversion
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
```

All panels use the shared `aruco_lib` library for ArUco operations.

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
| `-f, --format` | Paper format: A4 or Letter | required |
| `-w` | Markers in X direction | required |
| `-h` | Markers in Y direction | required |
| `-d` | Dictionary ID | 16 |
| `--dpi` | Print resolution | 300 |
| `--margin` | Page margin in mm | 10 |
| `--ratio` | Marker to cell size ratio | 0.8 |
| `--si` | Show generated image | false |

The tool outputs the physical dimensions and a ready-to-use `camera_calibration` command with all parameters.

## Troubleshooting

**Black video preview:** Ensure camera is not in use by another application.

**Pose estimation unavailable:** Load a calibration file first (button turns green when loaded).

**Poor detection:** Adjust lighting or try a different ArUco dictionary matching your markers.

**Build errors:** Verify vcpkg packages are installed with correct features:
```powershell
vcpkg list
# Should show: opencv4, imgui with dx11-binding and win32-binding
```
