#pragma once

#include "types.hpp"
#include <mutex>
#include <vector>

namespace aruco_lib {

class VideoSource {
public:
    VideoSource();
    ~VideoSource();

    // Open video source based on configuration
    bool open(const VideoSourceConfig& config);

    // Close the video source
    void close();

    // Check if video source is opened
    bool isOpened() const;

    // Get a frame from the video source (thread-safe)
    bool getFrame(cv::Mat& frame);

    // Get frame size
    cv::Size getFrameSize() const;

    // Get FPS
    double getFPS() const;

    // Get current configuration
    const VideoSourceConfig& getConfig() const { return config_; }

    // Enumerate available cameras (Windows-specific)
    static std::vector<CameraInfo> enumerateCameras();

    // Get test video path (relative to executable)
    static std::string getTestVideoPath();

    // Get test image path (relative to executable)
    static std::string getTestImagePath();

    // Get default calibration file path
    static std::string getDefaultCalibrationPath();

private:
    cv::VideoCapture capture_;
    mutable std::mutex mutex_;
    VideoSourceConfig config_;
};

} // namespace aruco_lib
