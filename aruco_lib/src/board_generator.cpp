#include "aruco_lib/board_generator.hpp"
#include <algorithm>
#include <cmath>

namespace aruco_lib {

cv::Mat BoardGenerator::generate(const BoardParams& params) {
    cv::Size imageSize = calculateImageSize(params);

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::aruco::GridBoard board(
        cv::Size(params.markersX, params.markersY),
        static_cast<float>(params.markerLengthPixels),
        static_cast<float>(params.markerSeparationPixels),
        dictionary);

    cv::Mat boardImage;
    board.generateImage(imageSize, boardImage, params.margins, params.borderBits);
    return boardImage;
}

cv::Mat BoardGenerator::generateCharuco(const CharucoBoardParams& params) {
    cv::Size imageSize = calculateCharucoImageSize(params);

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::aruco::CharucoBoard board(
        cv::Size(params.squaresX, params.squaresY),
        static_cast<float>(params.squareLengthPixels),
        static_cast<float>(params.markerLengthPixels),
        dictionary);

    cv::Mat boardImage;
    board.generateImage(imageSize, boardImage, params.margins, params.borderBits);
    return boardImage;
}

CalibrationBoardResult BoardGenerator::calculateCalibrationBoardSize(const CalibrationBoardParams& params) {
    if (params.boardType == BoardType::ChArUco) {
        return calculateCharucoCalibrationBoardSize(params);
    }
    return calculateArucoCalibrationBoardSize(params);
}

CalibrationBoardResult BoardGenerator::calculateArucoCalibrationBoardSize(const CalibrationBoardParams& params) {
    CalibrationBoardResult result;
    result.boardType = BoardType::ArUco;

    // Get paper dimensions
    int formatIdx = static_cast<int>(params.paperFormat);
    double paperWidthMm = PAPER_WIDTH_MM[formatIdx];
    double paperHeightMm = PAPER_HEIGHT_MM[formatIdx];

    // Calculate pixel dimensions based on DPI (1 inch = 25.4 mm)
    double pixelsPerMm = params.dpi / 25.4;
    result.pageWidthPx = static_cast<int>(paperWidthMm * pixelsPerMm);
    result.pageHeightPx = static_cast<int>(paperHeightMm * pixelsPerMm);
    int marginPx = static_cast<int>(params.marginMm * pixelsPerMm);

    // Calculate available area for the board
    int availableWidthPx = result.pageWidthPx - 2 * marginPx;
    int availableHeightPx = result.pageHeightPx - 2 * marginPx;

    // Calculate cell size to fit the grid
    double cellSizeFromWidth = availableWidthPx / (params.squaresX + (1.0 - params.markerRatio));
    double cellSizeFromHeight = availableHeightPx / (params.squaresY + (1.0 - params.markerRatio));
    double cellSizePx = std::min(cellSizeFromWidth, cellSizeFromHeight);

    int markerLengthPx = static_cast<int>(cellSizePx * params.markerRatio);
    int separationPx = static_cast<int>(cellSizePx * (1.0 - params.markerRatio));

    // Calculate actual board dimensions
    int boardWidthPx = params.squaresX * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;
    int boardHeightPx = params.squaresY * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;

    // Calculate physical dimensions in mm
    result.markerLengthMm = markerLengthPx / pixelsPerMm;
    result.squareLengthMm = separationPx / pixelsPerMm;  // For ArUco, this is the separation
    result.boardWidthMm = boardWidthPx / pixelsPerMm;
    result.boardHeightMm = boardHeightPx / pixelsPerMm;

    return result;
}

CalibrationBoardResult BoardGenerator::calculateCharucoCalibrationBoardSize(const CalibrationBoardParams& params) {
    CalibrationBoardResult result;
    result.boardType = BoardType::ChArUco;

    // Get paper dimensions
    int formatIdx = static_cast<int>(params.paperFormat);
    double paperWidthMm = PAPER_WIDTH_MM[formatIdx];
    double paperHeightMm = PAPER_HEIGHT_MM[formatIdx];

    // Calculate pixel dimensions based on DPI
    double pixelsPerMm = params.dpi / 25.4;
    result.pageWidthPx = static_cast<int>(paperWidthMm * pixelsPerMm);
    result.pageHeightPx = static_cast<int>(paperHeightMm * pixelsPerMm);
    int marginPx = static_cast<int>(params.marginMm * pixelsPerMm);

    // Calculate available area
    int availableWidthPx = result.pageWidthPx - 2 * marginPx;
    int availableHeightPx = result.pageHeightPx - 2 * marginPx;

    // For ChArUco: board size = squaresX * squareLength, squaresY * squareLength
    double squareSizeFromWidth = static_cast<double>(availableWidthPx) / params.squaresX;
    double squareSizeFromHeight = static_cast<double>(availableHeightPx) / params.squaresY;
    double squareSizePx = std::min(squareSizeFromWidth, squareSizeFromHeight);

    int squareLengthPx = static_cast<int>(squareSizePx);
    int markerLengthPx = static_cast<int>(squareSizePx * params.markerRatio);

    // Calculate actual board dimensions
    int boardWidthPx = params.squaresX * squareLengthPx;
    int boardHeightPx = params.squaresY * squareLengthPx;

    // Calculate physical dimensions in mm
    result.squareLengthMm = squareLengthPx / pixelsPerMm;
    result.markerLengthMm = markerLengthPx / pixelsPerMm;
    result.boardWidthMm = boardWidthPx / pixelsPerMm;
    result.boardHeightMm = boardHeightPx / pixelsPerMm;

    return result;
}

CalibrationBoardResult BoardGenerator::generateCalibrationBoard(const CalibrationBoardParams& params) {
    if (params.boardType == BoardType::ChArUco) {
        return generateCharucoCalibrationBoard(params);
    }
    return generateArucoCalibrationBoard(params);
}

CalibrationBoardResult BoardGenerator::generateArucoCalibrationBoard(const CalibrationBoardParams& params) {
    CalibrationBoardResult result = calculateArucoCalibrationBoardSize(params);

    // Validate
    if (result.markerLengthMm < 1.0) {
        return result;
    }

    // Calculate pixel dimensions
    double pixelsPerMm = params.dpi / 25.4;
    int pageWidthPx = result.pageWidthPx;
    int pageHeightPx = result.pageHeightPx;

    // Recalculate pixel dimensions for board generation
    int markerLengthPx = static_cast<int>(result.markerLengthMm * pixelsPerMm);
    int separationPx = static_cast<int>(result.squareLengthMm * pixelsPerMm);
    int boardWidthPx = static_cast<int>(result.boardWidthMm * pixelsPerMm);
    int boardHeightPx = static_cast<int>(result.boardHeightMm * pixelsPerMm);

    // Create the ArUco board
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::aruco::GridBoard board(
        cv::Size(params.squaresX, params.squaresY),
        static_cast<float>(markerLengthPx),
        static_cast<float>(separationPx),
        dictionary);

    // Generate the board image
    cv::Mat boardImage;
    cv::Size imageSize(boardWidthPx, boardHeightPx);
    board.generateImage(imageSize, boardImage, separationPx, params.borderBits);

    // Create full page image with white background
    cv::Mat pageImage(pageHeightPx, pageWidthPx, CV_8UC1, cv::Scalar(255));

    // Calculate position to center the board on the page
    int offsetX = (pageWidthPx - boardWidthPx) / 2;
    int offsetY = (pageHeightPx - boardHeightPx) / 2;

    // Copy board to page
    boardImage.copyTo(pageImage(cv::Rect(offsetX, offsetY, boardWidthPx, boardHeightPx)));

    result.image = pageImage;
    return result;
}

CalibrationBoardResult BoardGenerator::generateCharucoCalibrationBoard(const CalibrationBoardParams& params) {
    CalibrationBoardResult result = calculateCharucoCalibrationBoardSize(params);

    // Validate
    if (result.markerLengthMm < 1.0) {
        return result;
    }

    // Calculate pixel dimensions
    double pixelsPerMm = params.dpi / 25.4;
    int pageWidthPx = result.pageWidthPx;
    int pageHeightPx = result.pageHeightPx;

    // Recalculate pixel dimensions for board generation
    int squareLengthPx = static_cast<int>(result.squareLengthMm * pixelsPerMm);
    int markerLengthPx = static_cast<int>(result.markerLengthMm * pixelsPerMm);
    int boardWidthPx = static_cast<int>(result.boardWidthMm * pixelsPerMm);
    int boardHeightPx = static_cast<int>(result.boardHeightMm * pixelsPerMm);

    // Create the ChArUco board
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::aruco::CharucoBoard board(
        cv::Size(params.squaresX, params.squaresY),
        static_cast<float>(squareLengthPx),
        static_cast<float>(markerLengthPx),
        dictionary);

    // Generate the board image
    cv::Mat boardImage;
    cv::Size imageSize(boardWidthPx, boardHeightPx);
    board.generateImage(imageSize, boardImage, 0, params.borderBits);

    // Create full page image with white background
    cv::Mat pageImage(pageHeightPx, pageWidthPx, CV_8UC1, cv::Scalar(255));

    // Calculate position to center the board on the page
    int offsetX = (pageWidthPx - boardWidthPx) / 2;
    int offsetY = (pageHeightPx - boardHeightPx) / 2;

    // Copy board to page
    boardImage.copyTo(pageImage(cv::Rect(offsetX, offsetY, boardWidthPx, boardHeightPx)));

    result.image = pageImage;
    return result;
}

bool BoardGenerator::save(const cv::Mat& image, const std::string& filename) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(filename, image);
}

cv::Size BoardGenerator::calculateImageSize(const BoardParams& params) {
    int width = params.markersX * (params.markerLengthPixels + params.markerSeparationPixels)
                - params.markerSeparationPixels + 2 * params.margins;
    int height = params.markersY * (params.markerLengthPixels + params.markerSeparationPixels)
                 - params.markerSeparationPixels + 2 * params.margins;
    return cv::Size(width, height);
}

cv::Size BoardGenerator::calculateCharucoImageSize(const CharucoBoardParams& params) {
    int width = params.squaresX * params.squareLengthPixels + 2 * params.margins;
    int height = params.squaresY * params.squareLengthPixels + 2 * params.margins;
    return cv::Size(width, height);
}

} // namespace aruco_lib
