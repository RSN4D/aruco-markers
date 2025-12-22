cmake_minimum_required(VERSION 3.16)
project(aruco_markers VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV with required components
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

# Common include directory
set(COMMON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/common/include)

# Add all subprojects
add_subdirectory(create_markers)
add_subdirectory(detect_marker)
add_subdirectory(camera_calibration)
add_subdirectory(pose_estimation)
add_subdirectory(draw_cube)

# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT detect_markers)
