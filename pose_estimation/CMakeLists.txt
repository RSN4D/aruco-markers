cmake_minimum_required(VERSION 3.16)
project(pose_estimation_proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

add_executable(pose_estimation src/main.cpp)
target_include_directories(pose_estimation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)
target_link_libraries(pose_estimation PRIVATE ${OpenCV_LIBRARIES})

# Platform-specific optimizations
if(MSVC)
    target_compile_options(pose_estimation PRIVATE /O2)
else()
    target_compile_options(pose_estimation PRIVATE -O3)
endif()
