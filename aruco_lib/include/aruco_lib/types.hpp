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

// Board type selection
enum class BoardType {
    ArUco,
    ChArUco
};

inline const char* BOARD_TYPE_NAMES[] = {
    "ArUco Grid",
    "ChArUco"
};
constexpr int BOARD_TYPE_COUNT = 2;

// Parameters for ArUco board generation
struct BoardParams {
    int dictionaryId = 16;
    int markersX = 4;
    int markersY = 2;
    int markerLengthPixels = 200;
    int markerSeparationPixels = 100;
    int margins = 100;
    int borderBits = 1;
};

// Parameters for ChArUco board generation (pixel-based)
struct CharucoBoardParams {
    int dictionaryId = 16;
    int squaresX = 5;       // Number of chessboard squares in X
    int squaresY = 7;       // Number of chessboard squares in Y
    int squareLengthPixels = 200;
    int markerLengthPixels = 150;  // Must be less than squareLength
    int margins = 50;
    int borderBits = 1;
};

// Paper format for calibration boards
enum class PaperFormat {
    A4,
    A3,
    A2,
    ANSI_A,  // Letter
    ANSI_B,
    ANSI_C
};

// Paper format names for UI
inline const char* PAPER_FORMAT_NAMES[] = {
    "A4 (210 x 297 mm)",
    "A3 (297 x 420 mm)",
    "A2 (420 x 594 mm)",
    "ANSI A / Letter (8.5 x 11 in)",
    "ANSI B (11 x 17 in)",
    "ANSI C (17 x 22 in)"
};
constexpr int PAPER_FORMAT_COUNT = 6;

// Paper dimensions in mm
inline const double PAPER_WIDTH_MM[] = {
    210.0,   // A4
    297.0,   // A3
    420.0,   // A2
    215.9,   // ANSI A (Letter): 8.5 in
    279.4,   // ANSI B: 11 in
    431.8    // ANSI C: 17 in
};
inline const double PAPER_HEIGHT_MM[] = {
    297.0,   // A4
    420.0,   // A3
    594.0,   // A2
    279.4,   // ANSI A (Letter): 11 in
    431.8,   // ANSI B: 17 in
    558.8    // ANSI C: 22 in
};

// Parameters for calibration board generation (print-ready)
struct CalibrationBoardParams {
    BoardType boardType = BoardType::ArUco;
    int dictionaryId = 16;
    PaperFormat paperFormat = PaperFormat::A4;
    int squaresX = 5;       // For ArUco: markersX, for ChArUco: squaresX
    int squaresY = 7;       // For ArUco: markersY, for ChArUco: squaresY
    int dpi = 300;
    double marginMm = 10.0;
    double markerRatio = 0.8;  // For ArUco: marker/(marker+sep), for ChArUco: marker/square
    int borderBits = 1;
};

// Result of calibration board generation with physical dimensions
struct CalibrationBoardResult {
    cv::Mat image;
    BoardType boardType = BoardType::ArUco;
    double markerLengthMm = 0.0;
    double squareLengthMm = 0.0;   // For ChArUco: square size; For ArUco: used as separation
    double boardWidthMm = 0.0;
    double boardHeightMm = 0.0;
    int pageWidthPx = 0;
    int pageHeightPx = 0;

    // Get dimensions in meters for calibration tool
    double markerLengthMeters() const { return markerLengthMm / 1000.0; }
    double squareLengthMeters() const { return squareLengthMm / 1000.0; }
    // For ArUco compatibility (separation = squareLength for ArUco boards)
    double separationMm() const { return squareLengthMm; }
    double separationMeters() const { return squareLengthMm / 1000.0; }
};

// Parameters for camera calibration
struct CalibrationParams {
    BoardType boardType = BoardType::ArUco;
    int dictionaryId = 16;
    int squaresX = 5;           // For ArUco: markersX, for ChArUco: squaresX
    int squaresY = 7;           // For ArUco: markersY, for ChArUco: squaresY
    float markerLengthMeters = 0.04f;
    float squareLengthMeters = 0.05f;  // For ChArUco: square size; For ArUco: marker + separation
    bool refindStrategy = false;
    bool zeroTangentDist = false;
    bool fixPrincipalPoint = false;
    float aspectRatio = 1.0f;
    bool fixAspectRatio = false;

    // Convenience for ArUco compatibility
    float markerSeparationMeters() const {
        return squareLengthMeters - markerLengthMeters;
    }
};

// Parameters for pose estimation and cube rendering
struct PoseParams {
    BoardType boardType = BoardType::ArUco;
    int dictionaryId = 16;
    float markerLengthMeters = 0.04f;
    // ChArUco board params (for board pose estimation)
    int squaresX = 5;
    int squaresY = 7;
    float squareLengthMeters = 0.05f;  // Must be > markerLengthMeters for ChArUco
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
    // ChArUco-specific: board pose (single rvec/tvec for whole board)
    cv::Vec3d boardRvec;
    cv::Vec3d boardTvec;
    bool hasBoardPose = false;
    // ChArUco corner data
    std::vector<int> charucoIds;
    std::vector<cv::Point2f> charucoCorners;
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
