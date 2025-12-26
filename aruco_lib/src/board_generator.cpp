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

CalibrationBoardResult BoardGenerator::calculateCalibrationBoardSize(const CalibrationBoardParams& params) {
    CalibrationBoardResult result;

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
    // cellSize = markerSize + separation
    // markerSize = cellSize * ratio
    // separation = cellSize * (1 - ratio)
    // For the board: width = markersX * cellSize + separation (using separation as board margin)
    double cellSizeFromWidth = availableWidthPx / (params.markersX + (1.0 - params.markerRatio));
    double cellSizeFromHeight = availableHeightPx / (params.markersY + (1.0 - params.markerRatio));
    double cellSizePx = std::min(cellSizeFromWidth, cellSizeFromHeight);

    int markerLengthPx = static_cast<int>(cellSizePx * params.markerRatio);
    int separationPx = static_cast<int>(cellSizePx * (1.0 - params.markerRatio));

    // Calculate actual board dimensions
    int boardWidthPx = params.markersX * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;
    int boardHeightPx = params.markersY * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;

    // Calculate physical dimensions in mm
    result.markerLengthMm = markerLengthPx / pixelsPerMm;
    result.separationMm = separationPx / pixelsPerMm;
    result.boardWidthMm = boardWidthPx / pixelsPerMm;
    result.boardHeightMm = boardHeightPx / pixelsPerMm;

    return result;
}

CalibrationBoardResult BoardGenerator::generateCalibrationBoard(const CalibrationBoardParams& params) {
    CalibrationBoardResult result = calculateCalibrationBoardSize(params);

    // Validate
    if (result.markerLengthMm < 1.0) {
        // Marker too small, return empty result
        return result;
    }

    // Get paper dimensions
    int formatIdx = static_cast<int>(params.paperFormat);
    double paperWidthMm = PAPER_WIDTH_MM[formatIdx];
    double paperHeightMm = PAPER_HEIGHT_MM[formatIdx];

    // Calculate pixel dimensions
    double pixelsPerMm = params.dpi / 25.4;
    int pageWidthPx = result.pageWidthPx;
    int pageHeightPx = result.pageHeightPx;

    // Recalculate pixel dimensions for board generation
    int markerLengthPx = static_cast<int>(result.markerLengthMm * pixelsPerMm);
    int separationPx = static_cast<int>(result.separationMm * pixelsPerMm);
    int boardWidthPx = static_cast<int>(result.boardWidthMm * pixelsPerMm);
    int boardHeightPx = static_cast<int>(result.boardHeightMm * pixelsPerMm);

    // Create the ArUco board
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::aruco::GridBoard board(
        cv::Size(params.markersX, params.markersY),
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

} // namespace aruco_lib
