#pragma once

#include "types.hpp"

namespace aruco_lib {

class CubeRenderer {
public:
    CubeRenderer();
    ~CubeRenderer();

    // Configure with camera parameters
    void configure(const CameraParams& cameraParams);

    // Draw cubes on all detected markers
    void drawCubes(cv::Mat& frame, const DetectionResult& result, float markerLength);

    // Draw a single cube wireframe
    void drawCubeWireframe(cv::Mat& frame, const cv::Vec3d& rvec,
                           const cv::Vec3d& tvec, float size);

    // Check if properly configured
    bool isConfigured() const { return configured_; }

private:
    CameraParams cameraParams_;
    bool configured_ = false;
};

} // namespace aruco_lib
