cmake_minimum_required(VERSION 3.16)
project(aruco_markers VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV with required components
# find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

# Common include directory
set(COMMON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/common/include)

# Build the core library first
add_subdirectory(aruco_lib)

# Option to build GUI application (Windows only, requires ImGui)
option(BUILD_GUI_APP "Build ImGui GUI application (Windows only)" ON)
if(BUILD_GUI_APP AND WIN32)
    add_subdirectory(gui_app)
endif()

# Add CLI tool subprojects
add_subdirectory(create_markers)
add_subdirectory(detect_marker)
add_subdirectory(camera_calibration)
add_subdirectory(pose_estimation)
add_subdirectory(draw_cube)

# Set startup project for Visual Studio
if(BUILD_GUI_APP AND WIN32)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT aruco_gui)
else()
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT detect_markers)
endif()

# IDE folder organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
