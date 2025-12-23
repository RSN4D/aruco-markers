#pragma once

#include "types.hpp"
#include <memory>
#include <vector>

namespace aruco_lib {

class CameraCalibrator {
public:
    CameraCalibrator();
    ~CameraCalibrator();

    // Configure calibrator with parameters
    void configure(const CalibrationParams& params);

    // Process a frame and detect markers (returns true if markers detected)
    bool processFrame(const cv::Mat& frame, cv::Mat& outputFrame);

    // Capture current frame for calibration
    bool captureFrame(const cv::Mat& frame);

    // Get number of captured frames
    int getCapturedFrameCount() const;

    // Run calibration on captured frames
    bool calibrate();

    // Get reprojection error after calibration
    double getReprojectionError() const;

    // Get camera parameters after calibration
    CameraParams getCameraParams() const;

    // Save calibration to file
    bool saveCalibration(const std::string& filename);

    // Load calibration from file
    static CameraParams loadCalibration(const std::string& filename);

    // Load detector parameters from file
    bool loadDetectorParams(const std::string& filename);

    // Reset captured frames
    void reset();

    // Get current parameters
    const CalibrationParams& getParams() const { return params_; }

private:
    CalibrationParams params_;
    cv::aruco::DetectorParameters detectorParams_;
    std::unique_ptr<cv::aruco::GridBoard> board_;
    std::unique_ptr<cv::aruco::ArucoDetector> detector_;

    std::vector<std::vector<std::vector<cv::Point2f>>> allCorners_;
    std::vector<std::vector<int>> allIds_;
    cv::Size imageSize_;
    CameraParams cameraParams_;
    double reprojectionError_ = 0.0;

    void initBoard();
};

} // namespace aruco_lib
