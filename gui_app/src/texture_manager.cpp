#include "gui/texture_manager.hpp"

namespace gui {

TextureManager::TextureManager(ID3D11Device* device, ID3D11DeviceContext* context)
    : device_(device), context_(context) {
}

TextureManager::~TextureManager() {
    releaseAll();
}

ImTextureID TextureManager::updateTexture(const std::string& name, const cv::Mat& mat) {
    if (mat.empty()) {
        return ImTextureID{};
    }

    // Convert to RGBA
    cv::Mat rgba;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgba, cv::COLOR_BGR2RGBA);
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgba, cv::COLOR_GRAY2RGBA);
    } else {
        return ImTextureID{};
    }

    auto it = textures_.find(name);
    bool needsCreate = (it == textures_.end()) ||
                       (it->second.width != rgba.cols) ||
                       (it->second.height != rgba.rows);

    if (needsCreate) {
        if (it != textures_.end()) {
            textures_.erase(it);
        }

        TextureHandle handle;
        handle.width = rgba.cols;
        handle.height = rgba.rows;

        if (!createTexture(handle, rgba.cols, rgba.rows)) {
            return ImTextureID{};
        }

        textures_[name] = std::move(handle);
    }

    uploadTextureData(textures_[name], rgba);

    return reinterpret_cast<ImTextureID>(textures_[name].srv.Get());
}

void TextureManager::releaseTexture(const std::string& name) {
    textures_.erase(name);
}

void TextureManager::releaseAll() {
    textures_.clear();
}

bool TextureManager::getTextureSize(const std::string& name, int& width, int& height) const {
    auto it = textures_.find(name);
    if (it == textures_.end()) {
        return false;
    }
    width = it->second.width;
    height = it->second.height;
    return true;
}

bool TextureManager::createTexture(TextureHandle& handle, int width, int height) {
    if (!device_) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    if (FAILED(device_->CreateTexture2D(&desc, nullptr, &handle.texture))) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(device_->CreateShaderResourceView(handle.texture.Get(), &srvDesc, &handle.srv))) {
        return false;
    }

    return true;
}

void TextureManager::uploadTextureData(TextureHandle& handle, const cv::Mat& rgba) {
    if (!context_ || !handle.texture) return;

    context_->UpdateSubresource(
        handle.texture.Get(),
        0,
        nullptr,
        rgba.data,
        rgba.cols * 4,
        0
    );
}

} // namespace gui
