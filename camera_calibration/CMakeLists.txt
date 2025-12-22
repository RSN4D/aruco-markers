cmake_minimum_required(VERSION 3.16)
project(camera_calibration_proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

add_executable(camera_calibration src/main.cpp)
target_include_directories(camera_calibration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(camera_calibration PRIVATE ${OpenCV_LIBRARIES})

# Platform-specific optimizations
if(MSVC)
    target_compile_options(camera_calibration PRIVATE /O2)
else()
    target_compile_options(camera_calibration PRIVATE -O3)
endif()
