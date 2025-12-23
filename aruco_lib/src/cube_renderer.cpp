#include "aruco_lib/cube_renderer.hpp"

namespace aruco_lib {

CubeRenderer::CubeRenderer() = default;

CubeRenderer::~CubeRenderer() = default;

void CubeRenderer::configure(const CameraParams& cameraParams) {
    cameraParams_ = cameraParams;
    configured_ = cameraParams.isValid;
}

void CubeRenderer::drawCubes(cv::Mat& frame, const DetectionResult& result, float markerLength) {
    if (!configured_ || !result.hasPose) {
        return;
    }

    cv::aruco::drawDetectedMarkers(frame, result.corners, result.ids);

    for (size_t i = 0; i < result.ids.size(); i++) {
        drawCubeWireframe(frame, result.rvecs[i], result.tvecs[i], markerLength);
    }
}

void CubeRenderer::drawCubeWireframe(cv::Mat& frame, const cv::Vec3d& rvec,
                                     const cv::Vec3d& tvec, float l) {
    if (!configured_) {
        return;
    }

    float half_l = l / 2.0f;

    // Define cube corner points
    // Bottom face (on marker plane, z=0)
    // Top face (above marker, z=l)
    std::vector<cv::Point3f> cubePoints = {
        cv::Point3f( half_l,  half_l, l),  // 0: top front right
        cv::Point3f( half_l, -half_l, l),  // 1: top back right
        cv::Point3f(-half_l, -half_l, l),  // 2: top back left
        cv::Point3f(-half_l,  half_l, l),  // 3: top front left
        cv::Point3f( half_l,  half_l, 0),  // 4: bottom front right
        cv::Point3f( half_l, -half_l, 0),  // 5: bottom back right
        cv::Point3f(-half_l, -half_l, 0),  // 6: bottom back left
        cv::Point3f(-half_l,  half_l, 0)   // 7: bottom front left
    };

    std::vector<cv::Point2f> imagePoints;
    cv::projectPoints(cubePoints, rvec, tvec, cameraParams_.cameraMatrix,
                     cameraParams_.distCoeffs, imagePoints);

    cv::Scalar color(255, 0, 0);  // Blue in BGR
    int thickness = 3;

    // Draw top face
    cv::line(frame, imagePoints[0], imagePoints[1], color, thickness);
    cv::line(frame, imagePoints[1], imagePoints[2], color, thickness);
    cv::line(frame, imagePoints[2], imagePoints[3], color, thickness);
    cv::line(frame, imagePoints[3], imagePoints[0], color, thickness);

    // Draw bottom face
    cv::line(frame, imagePoints[4], imagePoints[5], color, thickness);
    cv::line(frame, imagePoints[5], imagePoints[6], color, thickness);
    cv::line(frame, imagePoints[6], imagePoints[7], color, thickness);
    cv::line(frame, imagePoints[7], imagePoints[4], color, thickness);

    // Draw vertical edges
    cv::line(frame, imagePoints[0], imagePoints[4], color, thickness);
    cv::line(frame, imagePoints[1], imagePoints[5], color, thickness);
    cv::line(frame, imagePoints[2], imagePoints[6], color, thickness);
    cv::line(frame, imagePoints[3], imagePoints[7], color, thickness);
}

} // namespace aruco_lib
