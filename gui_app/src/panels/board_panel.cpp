#include "gui/panels/board_panel.hpp"

namespace gui {

BoardPanel::BoardPanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Board", textures, video) {
    regeneratePreview();
}

void BoardPanel::render() {
    ImGui::Text("Generate ArUco Grid Board");
    ImGui::Separator();

    bool paramsChanged = false;

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", params_.dictionaryId)) {
        paramsChanged = true;
    }

    // Grid size
    ImGui::Text("Grid Size:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("W##grid", &params_.markersX, 1, 1)) {
        params_.markersX = std::clamp(params_.markersX, 1, 20);
        paramsChanged = true;
    }
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("H##grid", &params_.markersY, 1, 1)) {
        params_.markersY = std::clamp(params_.markersY, 1, 20);
        paramsChanged = true;
    }

    // Marker length
    if (renderIntSpinner("Marker Length (px)", params_.markerLengthPixels, 50, 1000, 10)) {
        paramsChanged = true;
    }

    // Separation
    if (renderIntSpinner("Separation (px)", params_.markerSeparationPixels, 10, 500, 10)) {
        paramsChanged = true;
    }

    // Margins
    if (renderIntSpinner("Margins (px)", params_.margins, 0, 500, 10)) {
        paramsChanged = true;
    }

    // Border bits
    if (renderIntSpinner("Border Bits", params_.borderBits, 1, 5)) {
        paramsChanged = true;
    }

    // Show calculated size
    cv::Size imgSize = aruco_lib::BoardGenerator::calculateImageSize(params_);
    ImGui::Text("Output Size: %d x %d pixels", imgSize.width, imgSize.height);

    ImGui::Separator();

    // Output path
    ImGui::Text("Output File:");
    ImGui::SameLine();
    char buf[512];
    strncpy_s(buf, outputPath_.c_str(), sizeof(buf) - 1);
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("##output", buf, sizeof(buf))) {
        outputPath_ = buf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse##output")) {
        std::string path;
        if (showSaveFileDialog(path, "PNG Files\0*.png\0JPEG Files\0*.jpg\0All Files\0*.*\0", "png")) {
            outputPath_ = path;
        }
    }

    ImGui::Separator();

    // Action buttons
    if (ImGui::Button("Generate & Save")) {
        regeneratePreview();
        if (!previewImage_.empty() && !outputPath_.empty()) {
            generator_.save(previewImage_, outputPath_);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Preview Only") || paramsChanged) {
        needsRegenerate_ = true;
    }

    // Regenerate if needed
    if (needsRegenerate_) {
        regeneratePreview();
        needsRegenerate_ = false;
    }

    ImGui::Separator();

    // Preview
    ImGui::Text("Preview:");
    renderPreviewImage("board_preview", previewImage_);
}

void BoardPanel::regeneratePreview() {
    previewImage_ = generator_.generate(params_);
}

} // namespace gui
