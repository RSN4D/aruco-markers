cmake_minimum_required(VERSION 3.16)
project(draw_cube_proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui calib3d objdetect videoio)

add_executable(draw_cube src/main.cpp)
target_include_directories(draw_cube PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)
target_link_libraries(draw_cube PRIVATE ${OpenCV_LIBRARIES})

# Platform-specific optimizations (Release only)
if(MSVC)
    target_compile_options(draw_cube PRIVATE $<$<CONFIG:Release>:/O2>)
else()
    target_compile_options(draw_cube PRIVATE $<$<CONFIG:Release>:-O3>)
endif()
