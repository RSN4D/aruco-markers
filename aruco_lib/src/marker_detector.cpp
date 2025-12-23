#include "aruco_lib/marker_detector.hpp"

namespace aruco_lib {

MarkerDetector::MarkerDetector() {
    initDetector();
}

MarkerDetector::~MarkerDetector() = default;

void MarkerDetector::setDictionary(int dictionaryId) {
    if (dictionaryId != dictionaryId_) {
        dictionaryId_ = dictionaryId;
        initDetector();
    }
}

void MarkerDetector::initDetector() {
    dictionary_ = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(dictionaryId_));
    detector_ = std::make_unique<cv::aruco::ArucoDetector>(dictionary_);
}

DetectionResult MarkerDetector::detect(const cv::Mat& frame) {
    DetectionResult result;

    if (frame.empty() || !detector_) {
        return result;
    }

    detector_->detectMarkers(frame, result.corners, result.ids);
    return result;
}

void MarkerDetector::drawDetectedMarkers(cv::Mat& frame, const DetectionResult& result) {
    if (!result.ids.empty()) {
        cv::aruco::drawDetectedMarkers(frame, result.corners, result.ids);
    }
}

} // namespace aruco_lib
