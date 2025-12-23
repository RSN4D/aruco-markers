#include "aruco_lib/board_generator.hpp"

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
