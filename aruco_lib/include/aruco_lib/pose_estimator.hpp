#pragma once

#include "types.hpp"
#include <memory>

namespace aruco_lib {

class PoseEstimator {
public:
    PoseEstimator();
    ~PoseEstimator();

    // Configure with pose parameters and camera parameters
    void configure(const PoseParams& params, const CameraParams& cameraParams);

    // Detect markers and estimate pose
    DetectionResult estimatePose(const cv::Mat& frame);

    // Draw pose axes on frame
    void drawPoseAxes(cv::Mat& frame, const DetectionResult& result, float axisLength = 0.1f);

    // Draw pose text overlay on frame
    void drawPoseText(cv::Mat& frame, const DetectionResult& result);

    // Check if properly configured
    bool isConfigured() const;

    // Get current parameters
    const PoseParams& getParams() const { return params_; }

private:
    PoseParams params_;
    CameraParams cameraParams_;
    std::unique_ptr<cv::aruco::ArucoDetector> detector_;
    std::vector<cv::Point3f> objectPoints_;
    bool configured_ = false;

    void initDetector();
    void initObjectPoints();
};

} // namespace aruco_lib
