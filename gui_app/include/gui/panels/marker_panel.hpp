#pragma once

#include "panel_base.hpp"

namespace gui {

class MarkerPanel : public PanelBase {
public:
    MarkerPanel(TextureManager& textures, VideoThread& video);
    void render() override;

private:
    aruco_lib::MarkerParams params_;
    aruco_lib::MarkerGenerator generator_;
    cv::Mat previewImage_;
    std::string outputPath_ = "marker.png";
    bool needsRegenerate_ = true;

    void regeneratePreview();
};

} // namespace gui
