#include "gui/panels/cube_panel.hpp"
#include <sstream>
#include <iomanip>

namespace gui {

CubePanel::CubePanel(TextureManager& textures, VideoThread& video)
    : PanelBase("Cube", textures, video) {
    videoConfig_.type = aruco_lib::VideoSourceType::TestVideo;
    calibrationPath_ = aruco_lib::VideoSource::getDefaultCalibrationPath();
}

void CubePanel::render() {
    ImGui::Text("Draw 3D Cube on Markers");
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
            startCapture();
        }
        if (!canStart) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("Load calibration first");
        }
    } else {
        if (ImGui::Button("Stop")) {
            stopCapture();
        }
    }

    ImGui::Separator();

    // Recording section
    ImGui::Text("Recording:");
    char buf[512];
    strncpy_s(buf, outputVideoPath_.c_str(), sizeof(buf) - 1);
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("##outputvideo", buf, sizeof(buf))) {
        outputVideoPath_ = buf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse##video")) {
        std::string path;
        if (showSaveFileDialog(path, "AVI Files\0*.avi\0MP4 Files\0*.mp4\0All Files\0*.*\0", "avi")) {
            outputVideoPath_ = path;
        }
    }

    if (!isRecording_) {
        bool canRecord = isRunning_ && !outputVideoPath_.empty();
        if (!canRecord) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Start Recording")) {
            startRecording();
        }
        if (!canRecord) {
            ImGui::EndDisabled();
        }
    } else {
        if (ImGui::Button("Stop Recording")) {
            stopRecording();
        }
        ImGui::SameLine();
        ImGui::Text("Recording: %d frames", recordedFrames_);
    }

    ImGui::Separator();

    // Video preview
    ImGui::Text("Preview:");
    if (isRunning_ && video_.getLatestFrame(displayFrame_)) {
        renderPreviewImage("cube_preview", displayFrame_);
    } else if (!displayFrame_.empty()) {
        renderPreviewImage("cube_preview", displayFrame_);
    } else {
        ImGui::TextDisabled("No video feed");
    }
}

void CubePanel::onActivate() {
    if (!calibrationLoaded_) {
        loadCalibration();
    }
}

void CubePanel::onDeactivate() {
    stopRecording();
    stopCapture();
}

void CubePanel::loadCalibration() {
    std::string path = useDefaultCalibration_
        ? aruco_lib::VideoSource::getDefaultCalibrationPath()
        : calibrationPath_;

    cameraParams_ = aruco_lib::CameraCalibrator::loadCalibration(path);
    calibrationLoaded_ = cameraParams_.isValid;

    if (calibrationLoaded_) {
        estimator_.configure(params_, cameraParams_);
        cubeRenderer_.configure(cameraParams_);
    }
}

void CubePanel::processFrame(cv::Mat& frame) {
    auto result = estimator_.estimatePose(frame);
    cubeRenderer_.drawCubes(frame, result, params_.markerLengthMeters);

    // Draw pose text for first marker
    if (result.hasPose && !result.ids.empty()) {
        const cv::Vec3d& tvec = result.tvecs[0];

        auto formatValue = [](const std::string& name, double value) {
            std::ostringstream oss;
            oss << name << ": " << std::fixed << std::setprecision(4) << value;
            return oss.str();
        };

        cv::Scalar color(0, 252, 124);
        cv::putText(frame, formatValue("x", tvec[0]), cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        cv::putText(frame, formatValue("y", tvec[1]), cv::Point(10, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        cv::putText(frame, formatValue("z", tvec[2]), cv::Point(10, 70),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    }

    // Write frame to video if recording
    if (isRecording_ && videoWriter_.isOpened()) {
        videoWriter_.write(frame);
        recordedFrames_++;
    }
}

void CubePanel::startCapture() {
    if (isRunning_ || !calibrationLoaded_) return;

    estimator_.configure(params_, cameraParams_);
    cubeRenderer_.configure(cameraParams_);

    video_.setFrameProcessor([this](cv::Mat& frame) {
        processFrame(frame);
    });

    video_.start(videoConfig_);
    isRunning_ = true;
}

void CubePanel::stopCapture() {
    if (!isRunning_) return;

    stopRecording();
    video_.stop();
    video_.clearFrameProcessor();
    isRunning_ = false;
}

void CubePanel::startRecording() {
    if (isRecording_ || !isRunning_) return;

    cv::Size frameSize = video_.getFrameSize();
    if (frameSize.width <= 0 || frameSize.height <= 0) {
        return;
    }

    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    double fps = 30.0;

    videoWriter_.open(outputVideoPath_, fourcc, fps, frameSize, true);
    if (videoWriter_.isOpened()) {
        isRecording_ = true;
        recordedFrames_ = 0;
    }
}

void CubePanel::stopRecording() {
    if (!isRecording_) return;

    videoWriter_.release();
    isRecording_ = false;
}

} // namespace gui
