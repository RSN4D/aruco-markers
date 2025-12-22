cmake_minimum_required(VERSION 3.16)
project(detect_markers_proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui objdetect videoio)

add_executable(detect_markers src/main.cpp)
target_include_directories(detect_markers PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)
target_link_libraries(detect_markers PRIVATE ${OpenCV_LIBRARIES})

# Platform-specific optimizations
if(MSVC)
    target_compile_options(detect_markers PRIVATE /O2)
else()
    target_compile_options(detect_markers PRIVATE -O3)
endif()
