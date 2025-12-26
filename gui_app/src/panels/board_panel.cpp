#include "gui/panels/board_panel.hpp"
#include <sstream>
#include <iomanip>

namespace gui {

BoardPanel::BoardPanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Board", textures, video) {
    regeneratePreview();
}

void BoardPanel::render() {
    ImGui::Text("Generate ArUco Grid Board");
    ImGui::Separator();

    // Mode selection tabs
    if (ImGui::BeginTabBar("BoardModeTab")) {
        if (ImGui::BeginTabItem("Custom Board")) {
            if (boardMode_ != 0) {
                boardMode_ = 0;
                needsRegenerate_ = true;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Calibration Board")) {
            if (boardMode_ != 1) {
                boardMode_ = 1;
                needsRegenerate_ = true;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    if (boardMode_ == 0) {
        renderCustomBoardMode();
    } else {
        renderCalibrationBoardMode();
    }
}

void BoardPanel::renderCustomBoardMode() {
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

void BoardPanel::renderCalibrationBoardMode() {
    bool paramsChanged = false;

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", calibParams_.dictionaryId)) {
        paramsChanged = true;
    }

    // Paper format selection
    ImGui::Text("Paper Format:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    int formatIdx = static_cast<int>(calibParams_.paperFormat);
    if (ImGui::Combo("##paper", &formatIdx, aruco_lib::PAPER_FORMAT_NAMES, aruco_lib::PAPER_FORMAT_COUNT)) {
        calibParams_.paperFormat = static_cast<aruco_lib::PaperFormat>(formatIdx);
        paramsChanged = true;
    }

    // Grid size
    ImGui::Text("Grid Size:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("W##calibgrid", &calibParams_.markersX, 1, 1)) {
        calibParams_.markersX = std::clamp(calibParams_.markersX, 1, 20);
        paramsChanged = true;
    }
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("H##calibgrid", &calibParams_.markersY, 1, 1)) {
        calibParams_.markersY = std::clamp(calibParams_.markersY, 1, 20);
        paramsChanged = true;
    }

    // DPI
    ImGui::Text("Print DPI:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##dpi", &calibParams_.dpi, 50, 100)) {
        calibParams_.dpi = std::clamp(calibParams_.dpi, 72, 600);
        paramsChanged = true;
    }

    // Page margin
    ImGui::Text("Page Margin (mm):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    float marginMmF = static_cast<float>(calibParams_.marginMm);
    if (ImGui::InputFloat("##margin", &marginMmF, 1.0f, 5.0f, "%.1f")) {
        calibParams_.marginMm = std::clamp(static_cast<double>(marginMmF), 0.0, 50.0);
        paramsChanged = true;
    }

    // Marker ratio
    ImGui::Text("Marker Ratio:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    float ratioF = static_cast<float>(calibParams_.markerRatio);
    if (ImGui::SliderFloat("##ratio", &ratioF, 0.5f, 0.95f, "%.2f")) {
        calibParams_.markerRatio = static_cast<double>(ratioF);
        paramsChanged = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Ratio of marker size to cell size.\nHigher = larger markers, smaller gaps.");
    }

    // Border bits
    if (renderIntSpinner("Border Bits", calibParams_.borderBits, 1, 5)) {
        paramsChanged = true;
    }

    ImGui::Separator();

    // Calculate and show dimensions
    if (paramsChanged || calibResult_.markerLengthMm == 0.0) {
        calibResult_ = aruco_lib::BoardGenerator::calculateCalibrationBoardSize(calibParams_);
    }

    // Show calculated physical dimensions
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Physical Dimensions (for Calibration tool):");

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "  Marker Length: " << calibResult_.markerLengthMm << " mm";
    ImGui::Text("%s", oss.str().c_str());

    oss.str(""); oss.clear();
    oss << "  Separation: " << calibResult_.separationMm << " mm";
    ImGui::Text("%s", oss.str().c_str());

    oss.str(""); oss.clear();
    oss << "  Board Size: " << calibResult_.boardWidthMm << " x " << calibResult_.boardHeightMm << " mm";
    ImGui::Text("%s", oss.str().c_str());

    oss.str(""); oss.clear();
    oss << "  Image Size: " << calibResult_.pageWidthPx << " x " << calibResult_.pageHeightPx << " px";
    ImGui::Text("%s", oss.str().c_str());

    ImGui::Separator();

    // Show calibration tool parameters
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Calibration Tool Parameters:");

    oss.str(""); oss.clear();
    oss << std::fixed << std::setprecision(6);
    oss << "  -w=" << calibParams_.markersX
        << " -h=" << calibParams_.markersY
        << " -l=" << calibResult_.markerLengthMeters()
        << " -s=" << calibResult_.separationMeters()
        << " -d=" << calibParams_.dictionaryId;

    ImGui::TextWrapped("%s", oss.str().c_str());

    // Copy button for parameters
    if (ImGui::Button("Copy Parameters")) {
        oss.str(""); oss.clear();
        oss << std::fixed << std::setprecision(6);
        oss << "-w=" << calibParams_.markersX
            << " -h=" << calibParams_.markersY
            << " -l=" << calibResult_.markerLengthMeters()
            << " -s=" << calibResult_.separationMeters()
            << " -d=" << calibParams_.dictionaryId;
        ImGui::SetClipboardText(oss.str().c_str());
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(copies to clipboard)");

    ImGui::Separator();

    // Output path
    ImGui::Text("Output File:");
    ImGui::SameLine();
    char buf[512];
    strncpy_s(buf, outputPath_.c_str(), sizeof(buf) - 1);
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("##caliboutput", buf, sizeof(buf))) {
        outputPath_ = buf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse##caliboutput")) {
        std::string path;
        if (showSaveFileDialog(path, "PNG Files\0*.png\0JPEG Files\0*.jpg\0All Files\0*.*\0", "png")) {
            outputPath_ = path;
        }
    }

    ImGui::Separator();

    // Action buttons
    if (ImGui::Button("Generate & Save")) {
        regenerateCalibrationPreview();
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
        regenerateCalibrationPreview();
        needsRegenerate_ = false;
    }

    ImGui::Separator();

    // Preview
    ImGui::Text("Preview:");
    renderPreviewImage("calib_board_preview", previewImage_);
}

void BoardPanel::regeneratePreview() {
    if (boardMode_ == 0) {
        previewImage_ = generator_.generate(params_);
    } else {
        regenerateCalibrationPreview();
    }
}

void BoardPanel::regenerateCalibrationPreview() {
    calibResult_ = generator_.generateCalibrationBoard(calibParams_);
    previewImage_ = calibResult_.image;
}

} // namespace gui
