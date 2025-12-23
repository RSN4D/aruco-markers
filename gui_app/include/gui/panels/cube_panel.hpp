#pragma once

#include "panel_base.hpp"

namespace gui {

class CubePanel : public PanelBase {
public:
    CubePanel(TextureManager& textures, VideoThread& video);
    void render() override;
    void onActivate() override;
    void onDeactivate() override;

private:
    aruco_lib::PoseParams params_;
    aruco_lib::VideoSourceConfig videoConfig_;
    aruco_lib::CameraParams cameraParams_;
    aruco_lib::PoseEstimator estimator_;
    aruco_lib::CubeRenderer cubeRenderer_;
    std::string calibrationPath_;
    std::string outputVideoPath_ = "output.avi";

    bool isRunning_ = false;
    bool isRecording_ = false;
    bool calibrationLoaded_ = false;
    bool useDefaultCalibration_ = true;
    cv::Mat displayFrame_;
    cv::VideoWriter videoWriter_;
    int recordedFrames_ = 0;

    void loadCalibration();
    void processFrame(cv::Mat& frame);
    void startCapture();
    void stopCapture();
    void startRecording();
    void stopRecording();
};

} // namespace gui
