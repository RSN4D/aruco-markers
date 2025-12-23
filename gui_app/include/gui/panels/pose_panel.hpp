#pragma once

#include "panel_base.hpp"

namespace gui {

class PosePanel : public PanelBase {
public:
    PosePanel(TextureManager& textures, VideoThread& video);
    void render() override;
    void onActivate() override;
    void onDeactivate() override;

private:
    aruco_lib::PoseParams params_;
    aruco_lib::VideoSourceConfig videoConfig_;
    aruco_lib::CameraParams cameraParams_;
    aruco_lib::PoseEstimator estimator_;
    std::string calibrationPath_;
    aruco_lib::DetectionResult lastResult_;

    bool isRunning_ = false;
    bool calibrationLoaded_ = false;
    bool useDefaultCalibration_ = true;
    cv::Mat displayFrame_;

    void loadCalibration();
    void processFrame(cv::Mat& frame);
    void startEstimation();
    void stopEstimation();
};

} // namespace gui
