#pragma once

#include "panel_base.hpp"

namespace gui {

class DetectPanel : public PanelBase {
public:
    DetectPanel(TextureManager& textures, VideoThread& video);
    void render() override;
    void onActivate() override;
    void onDeactivate() override;

private:
    aruco_lib::VideoSourceConfig videoConfig_;
    aruco_lib::MarkerDetector detector_;
    int dictionaryId_ = 16;
    bool isRunning_ = false;
    int detectedCount_ = 0;
    cv::Mat displayFrame_;

    void processFrame(cv::Mat& frame);
    void startDetection();
    void stopDetection();
};

} // namespace gui
