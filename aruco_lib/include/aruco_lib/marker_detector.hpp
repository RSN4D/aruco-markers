#pragma once

#include "types.hpp"
#include <memory>

namespace aruco_lib {

class MarkerDetector {
public:
    MarkerDetector();
    ~MarkerDetector();

    // Set the dictionary to use for detection
    void setDictionary(int dictionaryId);

    // Detect markers in the given frame
    DetectionResult detect(const cv::Mat& frame);

    // Draw detected markers on the frame
    void drawDetectedMarkers(cv::Mat& frame, const DetectionResult& result);

    // Get current dictionary ID
    int getDictionaryId() const { return dictionaryId_; }

private:
    int dictionaryId_ = 16;
    cv::aruco::Dictionary dictionary_;
    std::unique_ptr<cv::aruco::ArucoDetector> detector_;

    void initDetector();
};

} // namespace aruco_lib
