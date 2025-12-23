#pragma once

#include "panel_base.hpp"

namespace gui {

class BoardPanel : public PanelBase {
public:
    BoardPanel(TextureManager& textures, VideoThread& video);
    void render() override;

private:
    aruco_lib::BoardParams params_;
    aruco_lib::BoardGenerator generator_;
    cv::Mat previewImage_;
    std::string outputPath_ = "board.png";
    bool needsRegenerate_ = true;

    void regeneratePreview();
};

} // namespace gui
