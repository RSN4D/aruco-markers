#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_board.hpp>
#include <string>
#include <vector>
#include <functional>

namespace aruco_lib {

// Dictionary names for UI dropdown
inline const char* DICTIONARY_NAMES[] = {
    "DICT_4X4_50",
    "DICT_4X4_100",
    "DICT_4X4_250",
    "DICT_4X4_1000",
    "DICT_5X5_50",
    "DICT_5X5_100",
    "DICT_5X5_250",
    "DICT_5X5_1000",
    "DICT_6X6_50",
    "DICT_6X6_100",
    "DICT_6X6_250",
    "DICT_6X6_1000",
    "DICT_7X7_50",
    "DICT_7X7_100",
    "DICT_7X7_250",
    "DICT_7X7_1000",
    "DICT_ARUCO_ORIGINAL"
};
constexpr int DICTIONARY_COUNT = 17;

// Parameters for single marker generation
struct MarkerParams {
    int dictionaryId = 16;  // DICT_ARUCO_ORIGINAL
    int markerId = 0;
    int markerSizePixels = 200;
    int borderBits = 1;
};

// Parameters for board generation
struct BoardParams {
    int dictionaryId = 16;
    int markersX = 4;
    int markersY = 2;
    int markerLengthPixels = 200;
    int markerSeparationPixels = 100;
    int margins = 100;
    int borderBits = 1;
};

// Parameters for camera calibration
struct CalibrationParams {
    int dictionaryId = 16;
    int markersX = 4;
    int markersY = 2;
    float markerLengthMeters = 0.04f;
    float markerSeparationMeters = 0.02f;
    bool refindStrategy = false;
    bool zeroTangentDist = false;
    bool fixPrincipalPoint = false;
    float aspectRatio = 1.0f;
    bool fixAspectRatio = false;
};

// Parameters for pose estimation and cube rendering
struct PoseParams {
    int dictionaryId = 16;
    float markerLengthMeters = 0.05f;
};

// Camera intrinsic parameters
struct CameraParams {
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    int imageWidth = 0;
    int imageHeight = 0;
    bool isValid = false;
};

// Detection result containing marker data and optional pose
struct DetectionResult {
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<cv::Vec3d> rvecs;
    std::vector<cv::Vec3d> tvecs;
    bool hasPose = false;
};

// Video source type enumeration
enum class VideoSourceType {
    Camera,
    File,
    TestVideo
};

// Video source configuration
struct VideoSourceConfig {
    VideoSourceType type = VideoSourceType::Camera;
    int cameraId = 0;
    std::string filePath;
};

// Camera info for enumeration
struct CameraInfo {
    int id;
    std::string name;
};

} // namespace aruco_lib
