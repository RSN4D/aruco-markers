cmake_minimum_required(VERSION 3.16)
project(aruco_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

set(ARUCO_LIB_SOURCES
    src/marker_generator.cpp
    src/board_generator.cpp
    src/marker_detector.cpp
    src/video_source.cpp
    src/camera_calibrator.cpp
    src/pose_estimator.cpp
    src/cube_renderer.cpp
)

set(ARUCO_LIB_HEADERS
    include/aruco_lib/aruco_lib.hpp
    include/aruco_lib/types.hpp
    include/aruco_lib/marker_generator.hpp
    include/aruco_lib/board_generator.hpp
    include/aruco_lib/marker_detector.hpp
    include/aruco_lib/video_source.hpp
    include/aruco_lib/camera_calibrator.hpp
    include/aruco_lib/pose_estimator.hpp
    include/aruco_lib/cube_renderer.hpp
)

add_library(aruco_lib STATIC ${ARUCO_LIB_SOURCES} ${ARUCO_LIB_HEADERS})

target_include_directories(aruco_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(aruco_lib PUBLIC ${OpenCV_LIBRARIES})

# Windows-specific libraries for camera enumeration
if(WIN32)
    target_link_libraries(aruco_lib PRIVATE strmiids ole32)
endif()

# Platform optimizations (Release only)
if(MSVC)
    target_compile_options(aruco_lib PRIVATE $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(aruco_lib PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# Set folder for IDE
set_target_properties(aruco_lib PROPERTIES FOLDER "Libraries")
