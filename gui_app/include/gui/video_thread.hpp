#pragma once

#include <aruco_lib/video_source.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace gui {

class VideoThread {
public:
    using FrameProcessor = std::function<void(cv::Mat&)>;

    VideoThread();
    ~VideoThread();

    // Start capturing from the given video source configuration
    void start(const aruco_lib::VideoSourceConfig& config);

    // Stop capturing
    void stop();

    // Check if running
    bool isRunning() const { return running_; }

    // Check if video source is opened
    bool isOpened() const;

    // Set processor function (called on each frame before display)
    void setFrameProcessor(FrameProcessor processor);

    // Clear processor function
    void clearFrameProcessor();

    // Get latest processed frame (thread-safe copy)
    bool getLatestFrame(cv::Mat& frame);

    // Get raw frame without processing
    bool getRawFrame(cv::Mat& frame);

    // Get frame size
    cv::Size getFrameSize() const;

    // Get FPS
    double getFPS() const;

    // Get current configuration
    const aruco_lib::VideoSourceConfig& getConfig() const { return config_; }

private:
    aruco_lib::VideoSource source_;
    aruco_lib::VideoSourceConfig config_;

    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};

    mutable std::mutex frameMutex_;
    cv::Mat latestFrame_;
    cv::Mat latestRawFrame_;
    bool hasNewFrame_ = false;

    std::mutex processorMutex_;
    FrameProcessor processor_;

    void threadLoop();
};

} // namespace gui
