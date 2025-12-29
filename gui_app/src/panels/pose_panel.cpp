#include "gui/panels/pose_panel.hpp"
#include <sstream>
#include <iomanip>

namespace gui {

PosePanel::PosePanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Pose", textures, video) {
    videoConfig_.type = aruco_lib::VideoSourceType::TestVideo;
    calibrationPath_ = aruco_lib::VideoSource::getDefaultCalibrationPath();
}

void PosePanel::render() {
    ImGui::Text("Pose Estimation");
    ImGui::Separator();

    // Helper to safely reconfigure while video may be running
    auto safeReconfigure = [this]() {
        if (!calibrationLoaded_) return;
        bool wasRunning = isRunning_;
        if (wasRunning) stopEstimation();
        estimator_.configure(params_, cameraParams_);
        if (wasRunning) startEstimation();
    };

    // Board type selection
    ImGui::Text("Board Type:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    int boardTypeIdx = static_cast<int>(params_.boardType);
    if (ImGui::Combo("##boardtype", &boardTypeIdx, aruco_lib::BOARD_TYPE_NAMES, aruco_lib::BOARD_TYPE_COUNT)) {
        params_.boardType = static_cast<aruco_lib::BoardType>(boardTypeIdx);
        safeReconfigure();
    }

    bool isCharuco = (params_.boardType == aruco_lib::BoardType::ChArUco);

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", params_.dictionaryId)) {
        safeReconfigure();
    }

    // ChArUco board parameters
    if (isCharuco) {
        ImGui::Text("Board Squares:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("W##squares", &params_.squaresX, 1, 1)) {
            params_.squaresX = std::clamp(params_.squaresX, 2, 20);
            safeReconfigure();
        }
        ImGui::SameLine();
        ImGui::Text("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("H##squares", &params_.squaresY, 1, 1)) {
            params_.squaresY = std::clamp(params_.squaresY, 2, 20);
            safeReconfigure();
        }

        if (renderFloatInput("Square Length (m)", params_.squareLengthMeters, 0.001f, 1.0f)) {
            safeReconfigure();
        }
    }

    // Marker length
    if (renderFloatInput("Marker Length (m)", params_.markerLengthMeters, 0.001f, 1.0f)) {
        safeReconfigure();
    }

    ImGui::Separator();

    // Calibration file
    ImGui::Text("Calibration File:");
    ImGui::Checkbox("Use Default", &useDefaultCalibration_);

    if (!useDefaultCalibration_) {
        char buf[512];
        strncpy_s(buf, calibrationPath_.c_str(), sizeof(buf) - 1);
        ImGui::SetNextItemWidth(300);
        if (ImGui::InputText("##calibpath", buf, sizeof(buf))) {
            calibrationPath_ = buf;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse##calib")) {
            std::string path;
            if (showOpenFileDialog(path, "YAML Files\0*.yml;*.yaml\0All Files\0*.*\0")) {
                calibrationPath_ = path;
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Calibration")) {
        loadCalibration();
    }

    ImGui::SameLine();
    if (calibrationLoaded_) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded");
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Not Loaded");
    }

    ImGui::Separator();

    // Video source selector
    renderVideoSourceSelector(videoConfig_, cachedCameras_);

    ImGui::Separator();

    // Control buttons
    if (!isRunning_) {
        bool canStart = calibrationLoaded_;
        if (!canStart) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Start")) {
            startEstimation();
        }
        if (!canStart) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("Load calibration first");
        }
    } else {
        if (ImGui::Button("Stop")) {
            stopEstimation();
        }
    }

    // Pose data display
    if (lastResult_.hasBoardPose) {
        // ChArUco board pose (at top-left corner origin)
        ImGui::Separator();
        ImGui::Text("ChArUco Board Pose (top-left corner origin):");

        const cv::Vec3d& tvec = lastResult_.boardTvec;
        const cv::Vec3d& rvec = lastResult_.boardRvec;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "X: " << tvec[0] << " m  Y: " << tvec[1] << " m  Z: " << tvec[2] << " m";
        ImGui::Text("%s", oss.str().c_str());

        oss.str("");
        oss << "Rx: " << rvec[0] << "  Ry: " << rvec[1] << "  Rz: " << rvec[2];
        ImGui::Text("%s", oss.str().c_str());

        ImGui::Text("Charuco corners detected: %d", static_cast<int>(lastResult_.charucoIds.size()));
    }
    else if (lastResult_.hasPose && !lastResult_.ids.empty()) {
        ImGui::Separator();
        ImGui::Text("Pose Data (First Marker ID: %d):", lastResult_.ids[0]);

        const cv::Vec3d& tvec = lastResult_.tvecs[0];
        const cv::Vec3d& rvec = lastResult_.rvecs[0];

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "X: " << tvec[0] << " m  Y: " << tvec[1] << " m  Z: " << tvec[2] << " m";
        ImGui::Text("%s", oss.str().c_str());

        oss.str("");
        oss << "Rx: " << rvec[0] << "  Ry: " << rvec[1] << "  Rz: " << rvec[2];
        ImGui::Text("%s", oss.str().c_str());
    }

    ImGui::Separator();

    // Video preview
    ImGui::Text("Preview:");
    if (isRunning_ && video_.getLatestFrame(displayFrame_)) {
        renderPreviewImage("pose_preview", displayFrame_);
    } else if (!displayFrame_.empty()) {
        renderPreviewImage("pose_preview", displayFrame_);
    } else {
        ImGui::TextDisabled("No video feed");
    }
}

void PosePanel::onActivate() {
    if (!calibrationLoaded_) {
        loadCalibration();
    }
}

void PosePanel::onDeactivate() {
    stopEstimation();
}

void PosePanel::loadCalibration() {
    std::string path = useDefaultCalibration_
        ? aruco_lib::VideoSource::getDefaultCalibrationPath()
        : calibrationPath_;

    cameraParams_ = aruco_lib::CameraCalibrator::loadCalibration(path);
    calibrationLoaded_ = cameraParams_.isValid;

    if (calibrationLoaded_) {
        estimator_.configure(params_, cameraParams_);
    }
}

void PosePanel::processFrame(cv::Mat& frame) {
    lastResult_ = estimator_.estimatePose(frame);
    estimator_.drawPoseAxes(frame, lastResult_, params_.markerLengthMeters);
    estimator_.drawPoseText(frame, lastResult_);
}

void PosePanel::startEstimation() {
    if (isRunning_ || !calibrationLoaded_) return;

    estimator_.configure(params_, cameraParams_);

    video_.setFrameProcessor([this](cv::Mat& frame) {
        processFrame(frame);
    });

    video_.start(videoConfig_);
    isRunning_ = true;
}

void PosePanel::stopEstimation() {
    if (!isRunning_) return;

    video_.stop();
    video_.clearFrameProcessor();
    isRunning_ = false;
}

} // namespace gui
