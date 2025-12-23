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

    // Dictionary selection
    if (renderDictionaryCombo("Dictionary", params_.dictionaryId)) {
        if (calibrationLoaded_) {
            estimator_.configure(params_, cameraParams_);
        }
    }

    // Marker length
    if (renderFloatInput("Marker Length (m)", params_.markerLengthMeters, 0.001f, 1.0f)) {
        if (calibrationLoaded_) {
            estimator_.configure(params_, cameraParams_);
        }
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
    if (lastResult_.hasPose && !lastResult_.ids.empty()) {
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
