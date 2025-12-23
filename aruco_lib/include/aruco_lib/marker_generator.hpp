#pragma once

#include "types.hpp"

namespace aruco_lib {

class MarkerGenerator {
public:
    // Generate a marker image with the given parameters
    cv::Mat generate(const MarkerParams& params);

    // Save marker image to file
    bool save(const cv::Mat& image, const std::string& filename);
};

} // namespace aruco_lib
