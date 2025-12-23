#include "aruco_lib/marker_generator.hpp"

namespace aruco_lib {

cv::Mat MarkerGenerator::generate(const MarkerParams& params) {
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(params.dictionaryId));

    cv::Mat markerImg;
    cv::aruco::generateImageMarker(dictionary, params.markerId,
                                   params.markerSizePixels, markerImg,
                                   params.borderBits);
    return markerImg;
}

bool MarkerGenerator::save(const cv::Mat& image, const std::string& filename) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(filename, image);
}

} // namespace aruco_lib
