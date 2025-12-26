#pragma once

#include "types.hpp"

namespace aruco_lib {

class BoardGenerator {
public:
    // Generate a board image with the given parameters (pixel-based)
    cv::Mat generate(const BoardParams& params);

    // Generate a print-ready calibration board for A4/Letter paper
    CalibrationBoardResult generateCalibrationBoard(const CalibrationBoardParams& params);

    // Save board image to file
    bool save(const cv::Mat& image, const std::string& filename);

    // Calculate the resulting image size for given parameters
    static cv::Size calculateImageSize(const BoardParams& params);

    // Calculate calibration board dimensions without generating image
    static CalibrationBoardResult calculateCalibrationBoardSize(const CalibrationBoardParams& params);
};

} // namespace aruco_lib
