#include "gui/panels/marker_panel.hpp"

namespace gui {

MarkerPanel::MarkerPanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Marker", textures, video) {
    regeneratePreview();
}

void MarkerPanel::render() {
    ImGui::Text("Generate ArUco Marker");
    ImGui::Separator();

    bool paramsChanged = false;

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", params_.dictionaryId)) {
        paramsChanged = true;
    }

    // Marker ID
    if (renderIntSpinner("Marker ID", params_.markerId, 0, 1000)) {
        paramsChanged = true;
    }

    // Marker size
    if (renderIntSpinner("Size (pixels)", params_.markerSizePixels, 50, 2000, 50)) {
        paramsChanged = true;
    }

    // Border bits
    if (renderIntSpinner("Border Bits", params_.borderBits, 1, 5)) {
        paramsChanged = true;
    }

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
    renderPreviewImage("marker_preview", previewImage_);
}

void MarkerPanel::regeneratePreview() {
    previewImage_ = generator_.generate(params_);
}

} // namespace gui
