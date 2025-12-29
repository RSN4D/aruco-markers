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

    if (params_.boardType == BoardType::ChArUco) {
        // Validate ChArUco parameters: marker must be smaller than square
        float squareLen = params_.squareLengthMeters;
        float markerLen = params_.markerLengthMeters;

        // Ensure marker is smaller than square (required by OpenCV)
        if (markerLen >= squareLen) {
            markerLen = squareLen * 0.8f;  // Use 80% of square size as fallback
        }

        charucoBoard_ = std::make_unique<cv::aruco::CharucoBoard>(
            cv::Size(params_.squaresX, params_.squaresY),
            squareLen,
            markerLen,
            dictionary);

        cv::aruco::CharucoParameters charucoParams;
        cv::aruco::DetectorParameters detectorParams;
        charucoDetector_ = std::make_unique<cv::aruco::CharucoDetector>(*charucoBoard_, charucoParams, detectorParams);
    } else {
        charucoBoard_.reset();
        charucoDetector_.reset();
    }
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
    if (params_.boardType == BoardType::ChArUco) {
        return estimatePoseCharuco(frame);
    }
    return estimatePoseAruco(frame);
}

DetectionResult PoseEstimator::estimatePoseAruco(const cv::Mat& frame) {
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

DetectionResult PoseEstimator::estimatePoseCharuco(const cv::Mat& frame) {
    DetectionResult result;

    if (!configured_ || !charucoDetector_ || !charucoBoard_) {
        return result;
    }

    std::vector<int> markerIds;
    std::vector<std::vector<cv::Point2f>> markerCorners;

    charucoDetector_->detectBoard(frame, result.charucoCorners, result.charucoIds, markerCorners, markerIds);

    // Store marker data for visualization
    result.ids = markerIds;
    result.corners = markerCorners;

    // Estimate individual marker poses
    if (!markerIds.empty()) {
        result.rvecs.resize(markerIds.size());
        result.tvecs.resize(markerIds.size());

        for (size_t i = 0; i < markerIds.size(); i++) {
            cv::solvePnP(objectPoints_, markerCorners[i],
                        cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                        result.rvecs[i], result.tvecs[i]);
        }
        result.hasPose = true;
    }

    // Estimate board pose if we have enough charuco corners
    if (result.charucoIds.size() >= 4) {
        std::vector<cv::Point3f> objPoints;
        std::vector<cv::Point2f> imgPoints;

        charucoBoard_->matchImagePoints(result.charucoCorners, result.charucoIds, objPoints, imgPoints);

        if (!objPoints.empty()) {
            cv::solvePnP(objPoints, imgPoints,
                        cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                        result.boardRvec, result.boardTvec);
            result.hasBoardPose = true;
        }
    }

    return result;
}

void PoseEstimator::drawPoseAxes(cv::Mat& frame, const DetectionResult& result, float axisLength) {
    if (!configured_) {
        return;
    }

    // Draw detected markers
    if (!result.corners.empty()) {
        cv::aruco::drawDetectedMarkers(frame, result.corners, result.ids);
    }

    // Draw charuco corners if available
    if (!result.charucoCorners.empty()) {
        cv::aruco::drawDetectedCornersCharuco(frame, result.charucoCorners, result.charucoIds);
    }

    // Draw board pose for ChArUco (single axis at board origin)
    if (result.hasBoardPose) {
        cv::drawFrameAxes(frame, cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                         result.boardRvec, result.boardTvec, axisLength * 2);
    }
    // Draw individual marker poses for ArUco
    else if (result.hasPose) {
        for (size_t i = 0; i < result.ids.size(); i++) {
            cv::drawFrameAxes(frame, cameraParams_.cameraMatrix, cameraParams_.distCoeffs,
                             result.rvecs[i], result.tvecs[i], axisLength);
        }
    }
}

void PoseEstimator::drawPoseText(cv::Mat& frame, const DetectionResult& result) {
    cv::Scalar color(0, 252, 124);
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.6;
    int thickness = 2;

    auto formatValue = [](const std::string& name, double value) {
        std::ostringstream oss;
        oss << name << ": " << std::fixed << std::setprecision(4) << value;
        return oss.str();
    };

    // Prefer board pose for ChArUco
    if (result.hasBoardPose) {
        const cv::Vec3d& tvec = result.boardTvec;

        cv::putText(frame, "ChArUco Board Pose:", cv::Point(10, 30),
                    fontFace, fontScale, color, thickness);
        cv::putText(frame, formatValue("x", tvec[0]), cv::Point(10, 55),
                    fontFace, fontScale, color, thickness);
        cv::putText(frame, formatValue("y", tvec[1]), cv::Point(10, 75),
                    fontFace, fontScale, color, thickness);
        cv::putText(frame, formatValue("z", tvec[2]), cv::Point(10, 95),
                    fontFace, fontScale, color, thickness);
    }
    else if (result.hasPose && !result.ids.empty()) {
        const cv::Vec3d& tvec = result.tvecs[0];

        cv::putText(frame, formatValue("x", tvec[0]), cv::Point(10, 30),
                    fontFace, fontScale, color, thickness);
        cv::putText(frame, formatValue("y", tvec[1]), cv::Point(10, 50),
                    fontFace, fontScale, color, thickness);
        cv::putText(frame, formatValue("z", tvec[2]), cv::Point(10, 70),
                    fontFace, fontScale, color, thickness);
    }
}

} // namespace aruco_lib
