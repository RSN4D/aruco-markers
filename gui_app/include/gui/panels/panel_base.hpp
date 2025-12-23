#pragma once

#include "../texture_manager.hpp"
#include "../video_thread.hpp"
#include <aruco_lib/aruco_lib.hpp>
#include <imgui.h>
#include <string>
#include <vector>

namespace gui {

class PanelBase {
public:
    PanelBase(const std::string& name, TextureManager& textures, VideoThread& video);
    virtual ~PanelBase() = default;

    const std::string& getName() const { return name_; }

    virtual void render() = 0;
    virtual void onActivate() {}
    virtual void onDeactivate() {}

protected:
    std::string name_;
    TextureManager& textures_;
    VideoThread& video_;

    // Common UI helpers
    bool renderDictionaryCombo(const char* label, int& dictionaryId);
    void renderVideoSourceSelector(aruco_lib::VideoSourceConfig& config,
                                   const std::vector<aruco_lib::CameraInfo>& cameras);
    void renderPreviewImage(const std::string& textureName, const cv::Mat& image,
                           float maxWidth = 0, float maxHeight = 0);
    bool renderIntSpinner(const char* label, int& value, int min = 0, int max = 10000, int step = 1);
    bool renderFloatInput(const char* label, float& value, float min = 0.0f, float max = 100.0f);

    // File dialog helpers (Windows native)
    bool showSaveFileDialog(std::string& outPath, const char* filter, const char* defaultExt);
    bool showOpenFileDialog(std::string& outPath, const char* filter);

    // Cached camera list
    static std::vector<aruco_lib::CameraInfo> cachedCameras_;
    static bool camerasEnumerated_;
    static void enumerateCamerasIfNeeded();
};

} // namespace gui
