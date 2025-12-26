cmake_minimum_required(VERSION 3.16)
project(create_markers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui objdetect)

# generate_marker executable
add_executable(generate_marker src/create_marker.cpp)
target_include_directories(generate_marker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(generate_marker PRIVATE ${OpenCV_LIBRARIES})

# generate_board executable
add_executable(generate_board src/create_board.cpp)
target_include_directories(generate_board PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(generate_board PRIVATE ${OpenCV_LIBRARIES})

# generate_calibration_board executable (A4/Letter print-ready boards)
add_executable(generate_calibration_board src/create_calibration_board.cpp)
target_include_directories(generate_calibration_board PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(generate_calibration_board PRIVATE ${OpenCV_LIBRARIES})

# Platform-specific optimizations
if(MSVC)
    target_compile_options(generate_marker PRIVATE /O2)
    target_compile_options(generate_board PRIVATE /O2)
    target_compile_options(generate_calibration_board PRIVATE /O2)
else()
    target_compile_options(generate_marker PRIVATE -O3)
    target_compile_options(generate_board PRIVATE -O3)
    target_compile_options(generate_calibration_board PRIVATE -O3)
endif()
