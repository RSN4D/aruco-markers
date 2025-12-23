#include "gui/video_thread.hpp"

namespace gui {

VideoThread::VideoThread() = default;

VideoThread::~VideoThread() {
    stop();
}

void VideoThread::start(const aruco_lib::VideoSourceConfig& config) {
    stop();

    config_ = config;
    stopRequested_ = false;

    if (!source_.open(config)) {
        return;
    }

    running_ = true;
    thread_ = std::thread(&VideoThread::threadLoop, this);
}

void VideoThread::stop() {
    if (running_) {
        stopRequested_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
        running_ = false;
    }
    source_.close();
}

bool VideoThread::isOpened() const {
    return source_.isOpened();
}

void VideoThread::setFrameProcessor(FrameProcessor processor) {
    std::lock_guard<std::mutex> lock(processorMutex_);
    processor_ = std::move(processor);
}

void VideoThread::clearFrameProcessor() {
    std::lock_guard<std::mutex> lock(processorMutex_);
    processor_ = nullptr;
}

bool VideoThread::getLatestFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (latestFrame_.empty()) {
        return false;
    }
    latestFrame_.copyTo(frame);
    hasNewFrame_ = false;
    return true;
}

bool VideoThread::getRawFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (latestRawFrame_.empty()) {
        return false;
    }
    latestRawFrame_.copyTo(frame);
    return true;
}

cv::Size VideoThread::getFrameSize() const {
    return source_.getFrameSize();
}

double VideoThread::getFPS() const {
    return source_.getFPS();
}

void VideoThread::threadLoop() {
    cv::Mat frame, processed;

    while (!stopRequested_) {
        if (!source_.getFrame(frame)) {
            // For video files, loop back to beginning
            if (config_.type == aruco_lib::VideoSourceType::File ||
                config_.type == aruco_lib::VideoSourceType::TestVideo) {
                source_.close();
                source_.open(config_);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Make a copy for raw frame
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            frame.copyTo(latestRawFrame_);
        }

        // Process frame if processor is set
        frame.copyTo(processed);
        {
            std::lock_guard<std::mutex> lock(processorMutex_);
            if (processor_) {
                processor_(processed);
            }
        }

        // Store processed frame
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            processed.copyTo(latestFrame_);
            hasNewFrame_ = true;
        }

        // Small delay to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace gui
