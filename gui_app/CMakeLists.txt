cmake_minimum_required(VERSION 3.16)
project(aruco_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(imgui CONFIG REQUIRED)

# GUI source files
set(GUI_SOURCES
    src/main.cpp
    src/texture_manager.cpp
    src/video_thread.cpp
    src/panels/panel_base.cpp
    src/panels/marker_panel.cpp
    src/panels/board_panel.cpp
    src/panels/detect_panel.cpp
    src/panels/calibration_panel.cpp
    src/panels/pose_panel.cpp
    src/panels/cube_panel.cpp
)

set(GUI_HEADERS
    include/gui/texture_manager.hpp
    include/gui/video_thread.hpp
    include/gui/panels/panel_base.hpp
    include/gui/panels/marker_panel.hpp
    include/gui/panels/board_panel.hpp
    include/gui/panels/detect_panel.hpp
    include/gui/panels/calibration_panel.hpp
    include/gui/panels/pose_panel.hpp
    include/gui/panels/cube_panel.hpp
)

add_executable(aruco_gui ${GUI_SOURCES} ${GUI_HEADERS})

target_include_directories(aruco_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# DirectX 11 libraries
set(DX11_LIBS
    d3d11
    dxgi
)

target_link_libraries(aruco_gui PRIVATE
    aruco_lib
    ${OpenCV_LIBRARIES}
    imgui::imgui
    ${DX11_LIBS}
    comdlg32
)

# MSVC specific settings
if(MSVC)
    target_compile_options(aruco_gui PRIVATE /O2)
endif()

# Copy test data and calibration files to output directory
add_custom_command(TARGET aruco_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/test_data"
        "$<TARGET_FILE_DIR:aruco_gui>/test_data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/calibration_params.yml"
        "$<TARGET_FILE_DIR:aruco_gui>/calibration_params.yml"
    COMMENT "Copying test data and calibration files..."
)

# Set folder for IDE
set_target_properties(aruco_gui PROPERTIES FOLDER "Applications")

# Source group for Visual Studio
source_group("Header Files\\gui" FILES
    include/gui/texture_manager.hpp
    include/gui/video_thread.hpp
)
source_group("Header Files\\gui\\panels" FILES
    include/gui/panels/panel_base.hpp
    include/gui/panels/marker_panel.hpp
    include/gui/panels/board_panel.hpp
    include/gui/panels/detect_panel.hpp
    include/gui/panels/calibration_panel.hpp
    include/gui/panels/pose_panel.hpp
    include/gui/panels/cube_panel.hpp
)
source_group("Source Files\\panels" FILES
    src/panels/panel_base.cpp
    src/panels/marker_panel.cpp
    src/panels/board_panel.cpp
    src/panels/detect_panel.cpp
    src/panels/calibration_panel.cpp
    src/panels/pose_panel.cpp
    src/panels/cube_panel.cpp
)
