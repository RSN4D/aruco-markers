#pragma once

#include "panel_base.hpp"

namespace gui {

class CalibrationPanel : public PanelBase {
public:
    CalibrationPanel(TextureManager& textures, VideoThread& video);
    void render() override;
    void onActivate() override;
    void onDeactivate() override;

private:
    aruco_lib::CalibrationParams params_;
    aruco_lib::VideoSourceConfig videoConfig_;
    aruco_lib::CameraCalibrator calibrator_;
    std::string outputPath_ = "calibration_params.yml";
    std::string detectorParamsPath_;

    bool isCapturing_ = false;
    int capturedFrames_ = 0;
    double reprojError_ = 0.0;
    bool calibrationDone_ = false;
    std::string statusMessage_;
    cv::Mat displayFrame_;
    cv::Mat lastRawFrame_;

    void processFrame(cv::Mat& frame);
    void startCapture();
    void stopCapture();
    void captureCurrentFrame();
    void runCalibration();
};

} // namespace gui
