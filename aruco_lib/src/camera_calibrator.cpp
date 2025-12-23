#include "aruco_lib/camera_calibrator.hpp"
#include <ctime>
#include <iostream>

namespace aruco_lib {

CameraCalibrator::CameraCalibrator() = default;

CameraCalibrator::~CameraCalibrator() = default;

void CameraCalibrator::configure(const CalibrationParams& params) {
    params_ = params;
    initBoard();
    reset();
}

void CameraCalibrator::initBoard() {
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params_.dictionaryId));

    board_ = std::make_unique<cv::aruco::GridBoard>(
        cv::Size(params_.markersX, params_.markersY),
        params_.markerLengthMeters,
        params_.markerSeparationMeters,
        dictionary);

    detector_ = std::make_unique<cv::aruco::ArucoDetector>(dictionary, detectorParams_);
}

bool CameraCalibrator::loadDetectorParams(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        return false;
    }

    fs["adaptiveThreshWinSizeMin"] >> detectorParams_.adaptiveThreshWinSizeMin;
    fs["adaptiveThreshWinSizeMax"] >> detectorParams_.adaptiveThreshWinSizeMax;
    fs["adaptiveThreshWinSizeStep"] >> detectorParams_.adaptiveThreshWinSizeStep;
    fs["adaptiveThreshConstant"] >> detectorParams_.adaptiveThreshConstant;
    fs["minMarkerPerimeterRate"] >> detectorParams_.minMarkerPerimeterRate;
    fs["maxMarkerPerimeterRate"] >> detectorParams_.maxMarkerPerimeterRate;
    fs["polygonalApproxAccuracyRate"] >> detectorParams_.polygonalApproxAccuracyRate;
    fs["minCornerDistanceRate"] >> detectorParams_.minCornerDistanceRate;
    fs["minDistanceToBorder"] >> detectorParams_.minDistanceToBorder;
    fs["minMarkerDistanceRate"] >> detectorParams_.minMarkerDistanceRate;
    fs["cornerRefinementMethod"] >> detectorParams_.cornerRefinementMethod;
    fs["cornerRefinementWinSize"] >> detectorParams_.cornerRefinementWinSize;
    fs["cornerRefinementMaxIterations"] >> detectorParams_.cornerRefinementMaxIterations;
    fs["cornerRefinementMinAccuracy"] >> detectorParams_.cornerRefinementMinAccuracy;
    fs["markerBorderBits"] >> detectorParams_.markerBorderBits;
    fs["perspectiveRemovePixelPerCell"] >> detectorParams_.perspectiveRemovePixelPerCell;
    fs["perspectiveRemoveIgnoredMarginPerCell"] >> detectorParams_.perspectiveRemoveIgnoredMarginPerCell;
    fs["maxErroneousBitsInBorderRate"] >> detectorParams_.maxErroneousBitsInBorderRate;
    fs["minOtsuStdDev"] >> detectorParams_.minOtsuStdDev;
    fs["errorCorrectionRate"] >> detectorParams_.errorCorrectionRate;

    // Reinitialize detector with new parameters
    if (board_) {
        cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
            static_cast<cv::aruco::PredefinedDictionaryType>(params_.dictionaryId));
        detector_ = std::make_unique<cv::aruco::ArucoDetector>(dictionary, detectorParams_);
    }

    return true;
}

bool CameraCalibrator::processFrame(const cv::Mat& frame, cv::Mat& outputFrame) {
    if (!detector_ || !board_) {
        return false;
    }

    frame.copyTo(outputFrame);

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<std::vector<cv::Point2f>> rejected;

    detector_->detectMarkers(frame, corners, ids, rejected);

    if (params_.refindStrategy) {
        detector_->refineDetectedMarkers(frame, *board_, corners, ids, rejected);
    }

    if (!ids.empty()) {
        cv::aruco::drawDetectedMarkers(outputFrame, corners, ids);
        return true;
    }

    return false;
}

bool CameraCalibrator::captureFrame(const cv::Mat& frame) {
    if (!detector_ || !board_) {
        return false;
    }

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<std::vector<cv::Point2f>> rejected;

    detector_->detectMarkers(frame, corners, ids, rejected);

    if (params_.refindStrategy) {
        detector_->refineDetectedMarkers(frame, *board_, corners, ids, rejected);
    }

    if (!ids.empty()) {
        allCorners_.push_back(corners);
        allIds_.push_back(ids);
        imageSize_ = frame.size();
        return true;
    }

    return false;
}

int CameraCalibrator::getCapturedFrameCount() const {
    return static_cast<int>(allCorners_.size());
}

bool CameraCalibrator::calibrate() {
    if (allCorners_.empty()) {
        return false;
    }

    int calibrationFlags = 0;
    if (params_.fixAspectRatio) {
        calibrationFlags |= cv::CALIB_FIX_ASPECT_RATIO;
    }
    if (params_.zeroTangentDist) {
        calibrationFlags |= cv::CALIB_ZERO_TANGENT_DIST;
    }
    if (params_.fixPrincipalPoint) {
        calibrationFlags |= cv::CALIB_FIX_PRINCIPAL_POINT;
    }

    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Mat> rvecs, tvecs;

    if (params_.fixAspectRatio) {
        cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
        cameraMatrix.at<double>(0, 0) = params_.aspectRatio;
    }

    // Get object and image points for calibration
    std::vector<std::vector<cv::Point3f>> objPoints;
    std::vector<std::vector<cv::Point2f>> imgPoints;

    for (size_t frame = 0; frame < allCorners_.size(); frame++) {
        std::vector<cv::Point3f> objPts;
        std::vector<cv::Point2f> imgPts;

        board_->matchImagePoints(allCorners_[frame], allIds_[frame], objPts, imgPts);

        if (!objPts.empty()) {
            objPoints.push_back(objPts);
            imgPoints.push_back(imgPts);
        }
    }

    if (objPoints.empty()) {
        return false;
    }

    reprojectionError_ = cv::calibrateCamera(objPoints, imgPoints, imageSize_,
                                              cameraMatrix, distCoeffs, rvecs, tvecs,
                                              calibrationFlags);

    cameraParams_.cameraMatrix = cameraMatrix;
    cameraParams_.distCoeffs = distCoeffs;
    cameraParams_.imageWidth = imageSize_.width;
    cameraParams_.imageHeight = imageSize_.height;
    cameraParams_.isValid = true;

    return true;
}

double CameraCalibrator::getReprojectionError() const {
    return reprojectionError_;
}

CameraParams CameraCalibrator::getCameraParams() const {
    return cameraParams_;
}

bool CameraCalibrator::saveCalibration(const std::string& filename) {
    if (!cameraParams_.isValid) {
        return false;
    }

    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        return false;
    }

    time_t tt;
    time(&tt);
    struct tm* t2 = localtime(&tt);
    char buf[1024];
    strftime(buf, sizeof(buf) - 1, "%c", t2);

    fs << "calibration_time" << buf;
    fs << "image_width" << cameraParams_.imageWidth;
    fs << "image_height" << cameraParams_.imageHeight;

    int flags = 0;
    if (params_.fixAspectRatio) {
        flags |= cv::CALIB_FIX_ASPECT_RATIO;
        fs << "aspectRatio" << params_.aspectRatio;
    }
    if (params_.zeroTangentDist) flags |= cv::CALIB_ZERO_TANGENT_DIST;
    if (params_.fixPrincipalPoint) flags |= cv::CALIB_FIX_PRINCIPAL_POINT;

    fs << "flags" << flags;
    fs << "camera_matrix" << cameraParams_.cameraMatrix;
    fs << "distortion_coefficients" << cameraParams_.distCoeffs;
    fs << "avg_reprojection_error" << reprojectionError_;

    return true;
}

CameraParams CameraCalibrator::loadCalibration(const std::string& filename) {
    CameraParams params;

    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        return params;
    }

    fs["camera_matrix"] >> params.cameraMatrix;
    fs["distortion_coefficients"] >> params.distCoeffs;
    fs["image_width"] >> params.imageWidth;
    fs["image_height"] >> params.imageHeight;

    params.isValid = !params.cameraMatrix.empty() && !params.distCoeffs.empty();

    return params;
}

void CameraCalibrator::reset() {
    allCorners_.clear();
    allIds_.clear();
    imageSize_ = cv::Size();
    cameraParams_ = CameraParams();
    reprojectionError_ = 0.0;
}

} // namespace aruco_lib
