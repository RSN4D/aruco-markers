#pragma once

#include "types.hpp"

namespace aruco_lib {

class BoardGenerator {
public:
    // Generate an ArUco board image with the given parameters (pixel-based)
    cv::Mat generate(const BoardParams& params);

    // Generate a ChArUco board image with the given parameters (pixel-based)
    cv::Mat generateCharuco(const CharucoBoardParams& params);

    // Generate a print-ready calibration board (ArUco or ChArUco based on params)
    CalibrationBoardResult generateCalibrationBoard(const CalibrationBoardParams& params);

    // Save board image to file
    bool save(const cv::Mat& image, const std::string& filename);

    // Calculate the resulting image size for ArUco board parameters
    static cv::Size calculateImageSize(const BoardParams& params);

    // Calculate the resulting image size for ChArUco board parameters
    static cv::Size calculateCharucoImageSize(const CharucoBoardParams& params);

    // Calculate calibration board dimensions without generating image
    static CalibrationBoardResult calculateCalibrationBoardSize(const CalibrationBoardParams& params);

private:
    // Generate ArUco calibration board
    CalibrationBoardResult generateArucoCalibrationBoard(const CalibrationBoardParams& params);

    // Generate ChArUco calibration board
    CalibrationBoardResult generateCharucoCalibrationBoard(const CalibrationBoardParams& params);

    // Calculate ArUco calibration board size
    static CalibrationBoardResult calculateArucoCalibrationBoardSize(const CalibrationBoardParams& params);

    // Calculate ChArUco calibration board size
    static CalibrationBoardResult calculateCharucoCalibrationBoardSize(const CalibrationBoardParams& params);
};

} // namespace aruco_lib
