#include "gui/panels/calibration_panel.hpp"
#include <sstream>
#include <iomanip>

namespace gui {

CalibrationPanel::CalibrationPanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Calibration", textures, video) {
    videoConfig_.type = aruco_lib::VideoSourceType::Camera;
    statusMessage_ = "Configure board parameters and start capture";
}

void CalibrationPanel::render() {
    ImGui::Text("Camera Calibration");
    ImGui::Separator();

    // Board configuration
    ImGui::Text("Board Configuration:");

    // Board type selection
    ImGui::Text("Board Type:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    int boardTypeIdx = static_cast<int>(params_.boardType);
    if (ImGui::Combo("##boardtype", &boardTypeIdx, aruco_lib::BOARD_TYPE_NAMES, aruco_lib::BOARD_TYPE_COUNT)) {
        params_.boardType = static_cast<aruco_lib::BoardType>(boardTypeIdx);
        calibrator_.configure(params_);
    }

    bool isCharuco = (params_.boardType == aruco_lib::BoardType::ChArUco);

    if (renderDictionaryCombo("Dictionary", params_.dictionaryId)) {
        calibrator_.configure(params_);
    }

    ImGui::Text(isCharuco ? "Squares:" : "Markers:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("W##grid", &params_.squaresX, 1, 1)) {
        params_.squaresX = std::clamp(params_.squaresX, 2, 20);
    }
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("H##grid", &params_.squaresY, 1, 1)) {
        params_.squaresY = std::clamp(params_.squaresY, 2, 20);
    }

    if (renderFloatInput("Marker Length (m)", params_.markerLengthMeters, 0.001f, 1.0f)) {}
    if (isCharuco) {
        if (renderFloatInput("Square Length (m)", params_.squareLengthMeters, 0.001f, 1.0f)) {}
    } else {
        // For ArUco, separation = squareLength - markerLength
        float separation = params_.squareLengthMeters - params_.markerLengthMeters;
        if (renderFloatInput("Separation (m)", separation, 0.001f, 1.0f)) {
            params_.squareLengthMeters = params_.markerLengthMeters + separation;
        }
    }

    ImGui::Separator();

    // Calibration options
    ImGui::Text("Calibration Options:");
    ImGui::Checkbox("Refine Detection", &params_.refindStrategy);
    ImGui::SameLine();
    ImGui::Checkbox("Zero Tangent Dist", &params_.zeroTangentDist);
    ImGui::Checkbox("Fix Principal Point", &params_.fixPrincipalPoint);
    ImGui::SameLine();
    ImGui::Checkbox("Fix Aspect Ratio", &params_.fixAspectRatio);

    if (params_.fixAspectRatio) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputFloat("##aspect", &params_.aspectRatio, 0.01f, 0.1f, "%.2f");
    }

    ImGui::Separator();

    // Output file
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
        if (showSaveFileDialog(path, "YAML Files\0*.yml;*.yaml\0All Files\0*.*\0", "yml")) {
            outputPath_ = path;
        }
    }

    ImGui::Separator();

    // Video source selector
    renderVideoSourceSelector(videoConfig_, cachedCameras_);

    ImGui::Separator();

    // Control buttons
    if (!isCapturing_) {
        if (ImGui::Button("Start Capture")) {
            startCapture();
        }
    } else {
        if (ImGui::Button("Stop Capture")) {
            stopCapture();
        }
    }

    ImGui::SameLine();
    ImGui::Text("Captured Frames: %d", capturedFrames_);

    // Capture and calibrate buttons
    if (isCapturing_) {
        ImGui::SameLine();
        if (ImGui::Button("Capture Frame (C)")) {
            captureCurrentFrame();
        }
    }

    if (capturedFrames_ > 0) {
        ImGui::SameLine();
        if (ImGui::Button("Calibrate")) {
            runCalibration();
        }
    }

    // Status
    ImGui::Separator();
    ImGui::Text("Status: %s", statusMessage_.c_str());
    if (calibrationDone_) {
        std::ostringstream oss;
        oss << "Reprojection Error: " << std::fixed << std::setprecision(4) << reprojError_ << " px";
        ImGui::Text("%s", oss.str().c_str());
    }

    ImGui::Separator();

    // Video preview
    ImGui::Text("Preview:");
    if (isCapturing_ && video_.getLatestFrame(displayFrame_)) {
        renderPreviewImage("calib_preview", displayFrame_);
    } else if (!displayFrame_.empty()) {
        renderPreviewImage("calib_preview", displayFrame_);
    } else {
        ImGui::TextDisabled("Start capture to see video");
    }

    // Handle keyboard shortcut
    if (isCapturing_ && ImGui::IsKeyPressed(ImGuiKey_C)) {
        captureCurrentFrame();
    }
}

void CalibrationPanel::onActivate() {
    calibrator_.configure(params_);
}

void CalibrationPanel::onDeactivate() {
    stopCapture();
}

void CalibrationPanel::processFrame(cv::Mat& frame) {
    cv::Mat output;
    bool detected = calibrator_.processFrame(frame, output);
    output.copyTo(frame);

    // Add instruction text
    cv::putText(frame, "Press 'Capture Frame' to add. 'Calibrate' when done.",
                cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(255, 0, 0), 2);

    bool isCharuco = (params_.boardType == aruco_lib::BoardType::ChArUco);
    if (detected) {
        std::string msg = isCharuco ? "ChArUco corners detected" : "Board detected";
        cv::putText(frame, msg,
                    cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 255, 0), 2);
    }

    // Store raw frame for capture
    video_.getRawFrame(lastRawFrame_);
}

void CalibrationPanel::startCapture() {
    if (isCapturing_) return;

    calibrator_.configure(params_);
    calibrator_.reset();
    capturedFrames_ = 0;
    calibrationDone_ = false;
    statusMessage_ = "Capturing... Position board and click 'Capture Frame'";

    video_.setFrameProcessor([this](cv::Mat& frame) {
        processFrame(frame);
    });

    video_.start(videoConfig_);
    isCapturing_ = true;
}

void CalibrationPanel::stopCapture() {
    if (!isCapturing_) return;

    video_.stop();
    video_.clearFrameProcessor();
    isCapturing_ = false;
    statusMessage_ = "Capture stopped";
}

void CalibrationPanel::captureCurrentFrame() {
    if (!isCapturing_) return;

    if (calibrator_.captureFrame(lastRawFrame_)) {
        capturedFrames_ = calibrator_.getCapturedFrameCount();
        statusMessage_ = "Frame captured! Total: " + std::to_string(capturedFrames_);
    } else {
        statusMessage_ = "Failed to capture - no markers detected";
    }
}

void CalibrationPanel::runCalibration() {
    stopCapture();

    statusMessage_ = "Calibrating...";

    if (calibrator_.calibrate()) {
        reprojError_ = calibrator_.getReprojectionError();
        calibrationDone_ = true;

        if (calibrator_.saveCalibration(outputPath_)) {
            statusMessage_ = "Calibration saved to " + outputPath_;
        } else {
            statusMessage_ = "Calibration done but failed to save file";
        }
    } else {
        statusMessage_ = "Calibration failed - not enough valid frames";
        calibrationDone_ = false;
    }
}

} // namespace gui
