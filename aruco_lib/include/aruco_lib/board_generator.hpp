#pragma once

#include "types.hpp"

namespace aruco_lib {

class BoardGenerator {
public:
    // Generate a board image with the given parameters
    cv::Mat generate(const BoardParams& params);

    // Save board image to file
    bool save(const cv::Mat& image, const std::string& filename);

    // Calculate the resulting image size for given parameters
    static cv::Size calculateImageSize(const BoardParams& params);
};

} // namespace aruco_lib
