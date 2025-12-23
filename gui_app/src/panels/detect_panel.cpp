#include "gui/panels/detect_panel.hpp"

namespace gui {

DetectPanel::DetectPanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Detect", textures, video) {
    videoConfig_.type = aruco_lib::VideoSourceType::TestVideo;
}

void DetectPanel::render() {
    ImGui::Text("Detect ArUco Markers");
    ImGui::Separator();

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", dictionaryId_)) {
        detector_.setDictionary(dictionaryId_);
    }

    ImGui::Separator();

    // Video source selector
    renderVideoSourceSelector(videoConfig_, cachedCameras_);

    ImGui::Separator();

    // Control buttons
    if (!isRunning_) {
        if (ImGui::Button("Start Detection")) {
            startDetection();
        }
    } else {
        if (ImGui::Button("Stop")) {
            stopDetection();
        }
    }

    // Status
    ImGui::SameLine();
    if (isRunning_) {
        ImGui::Text("Running - %d markers detected", detectedCount_);
    } else {
        ImGui::Text("Stopped");
    }

    ImGui::Separator();

    // Video preview
    ImGui::Text("Preview:");
    if (isRunning_ && video_.getLatestFrame(displayFrame_)) {
        renderPreviewImage("detect_preview", displayFrame_);
    } else if (!displayFrame_.empty()) {
        renderPreviewImage("detect_preview", displayFrame_);
    } else {
        ImGui::TextDisabled("No video feed");
    }
}

void DetectPanel::onActivate() {
    // Nothing special needed
}

void DetectPanel::onDeactivate() {
    stopDetection();
}

void DetectPanel::processFrame(cv::Mat& frame) {
    auto result = detector_.detect(frame);
    detectedCount_ = static_cast<int>(result.ids.size());
    detector_.drawDetectedMarkers(frame, result);
}

void DetectPanel::startDetection() {
    if (isRunning_) return;

    detector_.setDictionary(dictionaryId_);

    video_.setFrameProcessor([this](cv::Mat& frame) {
        processFrame(frame);
    });

    video_.start(videoConfig_);
    isRunning_ = true;
}

void DetectPanel::stopDetection() {
    if (!isRunning_) return;

    video_.stop();
    video_.clearFrameProcessor();
    isRunning_ = false;
}

} // namespace gui
