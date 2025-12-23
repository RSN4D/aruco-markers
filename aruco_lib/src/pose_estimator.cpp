#include "aruco_lib/pose_estimator.hpp"
#include <sstream>
#include <iomanip>

namespace aruco_lib {

PoseEstimator::PoseEstimator() = default;

PoseEstimator::~PoseEstimator() = default;

void PoseEstimator::configure(const PoseParams& params, const CameraParams& cameraParams) {
    params_ = params;
    cameraParams_ = cameraParams;
    configured_ = cameraParams.isValid;

    if (configured_) {
        initDetector();
        initObjectPoints();
    }
}

void PoseEstimator::initDetector() {
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params_.dictionaryId));
    detector_ = std::make_unique<cv::aruco::ArucoDetector>(dictionary);
}

void PoseEstimator::initObjectPoints() {
    float half_size = params_.markerLengthMeters / 2.0f;
    objectPoints_ = {
        cv::Point3f(-half_size,  half_size, 0),
        cv::Point3f( half_size,  half_size, 0),
        cv::Point3f( half_size, -half_size, 0),
        cv::Point3f(-half_size, -half_size, 0)
    };
}

bool PoseEstimator::isConfigured() const {
    return configured_;
}

DetectionResult PoseEstimator::estimatePose(const cv::Mat& frame) {
    DetectionResult result;

    if (!configured_ || !detector_) {
        return result;
    }

    detector_->detectMarkers(frame, result.corners, result.ids);

    if (!result.ids.empty()) {
        result.rvecs.resize(result.ids.size());
        result.tvecs.resize(result.ids.size());

        for (size_t i = 0; i < result.ids.size(); i++) {
            cv::solvePnP(objectPoints_, result.corners[i],
                        cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                        result.rvecs[i], result.tvecs[i]);
        }

        result.hasPose = true;
    }

    return result;
}

void PoseEstimator::drawPoseAxes(cv::Mat& frame, const DetectionResult& result, float axisLength) {
    if (!configured_ || !result.hasPose) {
        return;
    }

    cv::aruco::drawDetectedMarkers(frame, result.corners, result.ids);

    for (size_t i = 0; i < result.ids.size(); i++) {
        cv::drawFrameAxes(frame, cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                         result.rvecs[i], result.tvecs[i], axisLength);
    }
}

void PoseEstimator::drawPoseText(cv::Mat& frame, const DetectionResult& result) {
    if (!result.hasPose || result.ids.empty()) {
        return;
    }

    // Draw pose data for first marker
    const cv::Vec3d& tvec = result.tvecs[0];

    auto formatValue = [](const std::string& name, double value) {
        std::ostringstream oss;
        oss << name << ": " << std::fixed << std::setprecision(4) << value;
        return oss.str();
    };

    cv::Scalar color(0, 252, 124);
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.6;
    int thickness = 2;

    cv::putText(frame, formatValue("x", tvec[0]), cv::Point(10, 30),
                fontFace, fontScale, color, thickness);
    cv::putText(frame, formatValue("y", tvec[1]), cv::Point(10, 50),
                fontFace, fontScale, color, thickness);
    cv::putText(frame, formatValue("z", tvec[2]), cv::Point(10, 70),
                fontFace, fontScale, color, thickness);
}

} // namespace aruco_lib
