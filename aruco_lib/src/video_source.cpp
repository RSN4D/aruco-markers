#include "aruco_lib/video_source.hpp"
#include <filesystem>

#ifdef _WIN32
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
#endif

namespace aruco_lib {

VideoSource::VideoSource() = default;

VideoSource::~VideoSource() {
    close();
}

bool VideoSource::open(const VideoSourceConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    close();
    config_ = config;

    bool success = false;
    switch (config.type) {
        case VideoSourceType::Camera:
            success = capture_.open(config.cameraId);
            break;
        case VideoSourceType::File:
            success = capture_.open(config.filePath);
            break;
        case VideoSourceType::TestVideo:
            success = capture_.open(getTestVideoPath());
            break;
    }

    return success;
}

void VideoSource::close() {
    if (capture_.isOpened()) {
        capture_.release();
    }
}

bool VideoSource::isOpened() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return capture_.isOpened();
}

bool VideoSource::getFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!capture_.isOpened()) {
        return false;
    }

    if (!capture_.grab()) {
        return false;
    }

    return capture_.retrieve(frame);
}

cv::Size VideoSource::getFrameSize() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!capture_.isOpened()) {
        return cv::Size(0, 0);
    }

    int width = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
    return cv::Size(width, height);
}

double VideoSource::getFPS() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!capture_.isOpened()) {
        return 0.0;
    }

    return capture_.get(cv::CAP_PROP_FPS);
}

std::vector<CameraInfo> VideoSource::enumerateCameras() {
    std::vector<CameraInfo> cameras;

#ifdef _WIN32
    CoInitialize(nullptr);

    ICreateDevEnum* devEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, reinterpret_cast<void**>(&devEnum));

    if (SUCCEEDED(hr)) {
        IEnumMoniker* enumMoniker = nullptr;
        hr = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMoniker, 0);

        if (hr == S_OK && enumMoniker) {
            IMoniker* moniker = nullptr;
            int index = 0;

            while (enumMoniker->Next(1, &moniker, nullptr) == S_OK) {
                IPropertyBag* propBag = nullptr;
                hr = moniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag,
                                           reinterpret_cast<void**>(&propBag));

                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = propBag->Read(L"FriendlyName", &var, nullptr);

                    if (SUCCEEDED(hr) && var.vt == VT_BSTR) {
                        char name[256];
                        WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1,
                                          name, sizeof(name), nullptr, nullptr);

                        CameraInfo info;
                        info.id = index;
                        info.name = name;
                        cameras.push_back(info);
                    }

                    VariantClear(&var);
                    propBag->Release();
                }

                moniker->Release();
                index++;
            }

            enumMoniker->Release();
        }

        devEnum->Release();
    }

    CoUninitialize();
#else
    // Fallback for non-Windows: try opening cameras 0-9
    for (int i = 0; i < 10; i++) {
        cv::VideoCapture cap(i);
        if (cap.isOpened()) {
            CameraInfo info;
            info.id = i;
            info.name = "Camera " + std::to_string(i);
            cameras.push_back(info);
            cap.release();
        }
    }
#endif

    return cameras;
}

std::string VideoSource::getTestVideoPath() {
    // Try relative paths from executable location
    std::vector<std::string> paths = {
        "test_data/test_video.mp4",
        "../test_data/test_video.mp4",
        "../../test_data/test_video.mp4",
        "../../../test_data/test_video.mp4"
    };

    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return "test_data/test_video.mp4";
}

std::string VideoSource::getTestImagePath() {
    std::vector<std::string> paths = {
        "test_data/test_image.png",
        "../test_data/test_image.png",
        "../../test_data/test_image.png",
        "../../../test_data/test_image.png"
    };

    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return "test_data/test_image.png";
}

std::string VideoSource::getDefaultCalibrationPath() {
    std::vector<std::string> paths = {
        "calibration_params.yml",
        "../calibration_params.yml",
        "../../calibration_params.yml",
        "../../../calibration_params.yml"
    };

    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return "calibration_params.yml";
}

} // namespace aruco_lib
