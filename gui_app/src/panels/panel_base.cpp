#include "gui/panels/panel_base.hpp"
#include <Windows.h>
#include <commdlg.h>
#include <shlobj.h>

namespace gui {

std::vector<aruco_lib::CameraInfo> PanelBase::cachedCameras_;
bool PanelBase::camerasEnumerated_ = false;

PanelBase::PanelBase(const std::string& name, TextureManager& textures, VideoThread& video)
    : name_(name), textures_(textures), video_(video) {
    enumerateCamerasIfNeeded();
}

void PanelBase::enumerateCamerasIfNeeded() {
    if (!camerasEnumerated_) {
        cachedCameras_ = aruco_lib::VideoSource::enumerateCameras();
        camerasEnumerated_ = true;
    }
}

bool PanelBase::renderDictionaryCombo(const char* label, int& dictionaryId) {
    bool changed = false;
    if (ImGui::BeginCombo(label, aruco_lib::DICTIONARY_NAMES[dictionaryId])) {
        for (int i = 0; i < aruco_lib::DICTIONARY_COUNT; i++) {
            bool selected = (dictionaryId == i);
            if (ImGui::Selectable(aruco_lib::DICTIONARY_NAMES[i], selected)) {
                dictionaryId = i;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

void PanelBase::renderVideoSourceSelector(aruco_lib::VideoSourceConfig& config,
                                          const std::vector<aruco_lib::CameraInfo>& cameras) {
    ImGui::Text("Video Source:");

    // Camera option
    bool isCamera = (config.type == aruco_lib::VideoSourceType::Camera);
    if (ImGui::RadioButton("Camera", isCamera)) {
        config.type = aruco_lib::VideoSourceType::Camera;
    }

    if (isCamera) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);

        std::string currentCamera = "Camera " + std::to_string(config.cameraId);
        for (const auto& cam : cameras) {
            if (cam.id == config.cameraId) {
                currentCamera = cam.name;
                break;
            }
        }

        if (ImGui::BeginCombo("##camera", currentCamera.c_str())) {
            for (const auto& cam : cameras) {
                bool selected = (config.cameraId == cam.id);
                if (ImGui::Selectable(cam.name.c_str(), selected)) {
                    config.cameraId = cam.id;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    // Video file option
    bool isFile = (config.type == aruco_lib::VideoSourceType::File);
    if (ImGui::RadioButton("Video File", isFile)) {
        config.type = aruco_lib::VideoSourceType::File;
    }

    if (isFile) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300);
        char buf[512];
        strncpy_s(buf, config.filePath.c_str(), sizeof(buf) - 1);
        if (ImGui::InputText("##filepath", buf, sizeof(buf))) {
            config.filePath = buf;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse##video")) {
            std::string path;
            if (showOpenFileDialog(path, "Video Files\0*.mp4;*.avi;*.mkv;*.mov\0All Files\0*.*\0")) {
                config.filePath = path;
            }
        }
    }

    // Test video option
    bool isTest = (config.type == aruco_lib::VideoSourceType::TestVideo);
    if (ImGui::RadioButton("Test Video (test_video.mp4)", isTest)) {
        config.type = aruco_lib::VideoSourceType::TestVideo;
    }
}

void PanelBase::renderPreviewImage(const std::string& textureName, const cv::Mat& image,
                                   float maxWidth, float maxHeight) {
    if (image.empty()) {
        ImGui::TextDisabled("No image to display");
        return;
    }

    ImTextureID texId = textures_.updateTexture(textureName, image);
    if (!texId) {
        ImGui::TextDisabled("Failed to create texture");
        return;
    }

    float imgWidth = static_cast<float>(image.cols);
    float imgHeight = static_cast<float>(image.rows);

    // Calculate display size
    if (maxWidth <= 0) maxWidth = ImGui::GetContentRegionAvail().x;
    if (maxHeight <= 0) maxHeight = ImGui::GetContentRegionAvail().y - 20;

    float scaleX = maxWidth / imgWidth;
    float scaleY = maxHeight / imgHeight;
    float scale = std::min(scaleX, scaleY);
    scale = std::min(scale, 1.0f);  // Don't upscale

    float displayWidth = imgWidth * scale;
    float displayHeight = imgHeight * scale;

    ImGui::Image(texId, ImVec2(displayWidth, displayHeight));
}

bool PanelBase::renderIntSpinner(const char* label, int& value, int min, int max, int step) {
    bool changed = false;

    ImGui::PushID(label);
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt(label, &value, step, step * 10)) {
        value = std::clamp(value, min, max);
        changed = true;
    }
    ImGui::PopID();

    return changed;
}

bool PanelBase::renderFloatInput(const char* label, float& value, float min, float max) {
    bool changed = false;

    ImGui::PushID(label);
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputFloat(label, &value, 0.001f, 0.01f, "%.4f")) {
        value = std::clamp(value, min, max);
        changed = true;
    }
    ImGui::PopID();

    return changed;
}

bool PanelBase::showSaveFileDialog(std::string& outPath, const char* filter, const char* defaultExt) {
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        outPath = filename;
        return true;
    }

    return false;
}

bool PanelBase::showOpenFileDialog(std::string& outPath, const char* filter) {
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        outPath = filename;
        return true;
    }

    return false;
}

} // namespace gui
