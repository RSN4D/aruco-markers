#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <unordered_map>
#include <string>

namespace gui {

using Microsoft::WRL::ComPtr;

class DX12Backend;

struct TextureHandle {
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    UINT srvIndex = 0;
    int width = 0;
    int height = 0;
};

class TextureManager {
public:
    TextureManager(DX12Backend* backend);
    ~TextureManager();

    // Create or update texture from OpenCV Mat
    // Returns ImTextureID for use with ImGui::Image()
    ImTextureID updateTexture(const std::string& name, const cv::Mat& mat);

    // Release a specific texture
    void releaseTexture(const std::string& name);

    // Release all textures
    void releaseAll();

    // Get texture dimensions
    bool getTextureSize(const std::string& name, int& width, int& height) const;

private:
    DX12Backend* backend_;
    std::unordered_map<std::string, TextureHandle> textures_;

    bool createTexture(TextureHandle& handle, int width, int height);
    void uploadTextureData(TextureHandle& handle, const cv::Mat& rgba);
};

} // namespace gui
