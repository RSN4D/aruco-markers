#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <unordered_map>
#include <string>

namespace gui {

using Microsoft::WRL::ComPtr;

struct TextureHandle {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    int width = 0;
    int height = 0;
};

class TextureManager {
public:
    TextureManager(ID3D11Device* device, ID3D11DeviceContext* context);
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
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    std::unordered_map<std::string, TextureHandle> textures_;

    bool createTexture(TextureHandle& handle, int width, int height);
    void uploadTextureData(TextureHandle& handle, const cv::Mat& rgba);
};

} // namespace gui
