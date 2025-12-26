#pragma once

#include "panel_base.hpp"

namespace gui {

class BoardPanel : public PanelBase {
public:
    BoardPanel(TextureManager& textures, VideoThread& video);
    void render() override;

private:
    // Mode selection: 0 = Custom Board, 1 = Calibration Board
    int boardMode_ = 0;

    // Custom board parameters (existing functionality)
    aruco_lib::BoardParams params_;

    // Calibration board parameters (new functionality)
    aruco_lib::CalibrationBoardParams calibParams_;
    aruco_lib::CalibrationBoardResult calibResult_;

    aruco_lib::BoardGenerator generator_;
    cv::Mat previewImage_;
    std::string outputPath_ = "board.png";
    bool needsRegenerate_ = true;

    void renderCustomBoardMode();
    void renderCalibrationBoardMode();
    void regeneratePreview();
    void regenerateCalibrationPreview();
};

} // namespace gui
