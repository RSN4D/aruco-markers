#include "gui/texture_manager.hpp"
#include "gui/dx12_backend.hpp"

namespace gui {

TextureManager::TextureManager(DX12Backend* backend)
    : backend_(backend) {
}

TextureManager::~TextureManager() {
    releaseAll();
}

ImTextureID TextureManager::updateTexture(const std::string& name, const cv::Mat& mat) {
    if (mat.empty() || !backend_) {
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
            // Wait for GPU to finish using the old texture before releasing
            backend_->waitForGpu();
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

    return (ImTextureID)(textures_[name].gpuHandle.ptr);
}

void TextureManager::releaseTexture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        backend_->waitForGpu();
        textures_.erase(it);
    }
}

void TextureManager::releaseAll() {
    if (backend_ && !textures_.empty()) {
        backend_->waitForGpu();
    }
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
    ID3D12Device* device = backend_->getDevice();
    if (!device) return false;

    // Create texture resource
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = 0;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&handle.texture)))) {
        return false;
    }

    // Calculate upload buffer size
    UINT64 uploadBufferSize = 0;
    device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    // Create upload buffer
    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Alignment = 0;
    uploadDesc.Width = uploadBufferSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.SampleDesc.Quality = 0;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    if (FAILED(device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&handle.uploadBuffer)))) {
        return false;
    }

    // Get SRV index and create descriptor
    handle.srvIndex = backend_->getNextSrvIndex();

    ID3D12DescriptorHeap* srvHeap = backend_->getSrvHeap();
    UINT descriptorSize = backend_->getSrvDescriptorSize();

    handle.cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.cpuHandle.ptr += handle.srvIndex * descriptorSize;

    handle.gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    handle.gpuHandle.ptr += handle.srvIndex * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(handle.texture.Get(), &srvDesc, handle.cpuHandle);

    // Texture starts in COPY_DEST state
    handle.isInShaderResourceState = false;

    return true;
}

void TextureManager::uploadTextureData(TextureHandle& handle, const cv::Mat& rgba) {
    if (!backend_ || !handle.texture || !handle.uploadBuffer) return;

    ID3D12Device* device = backend_->getDevice();
    ID3D12GraphicsCommandList* cmdList = backend_->getCommandList();

    // Transition from shader resource to copy dest if needed
    if (handle.isInShaderResourceState) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = handle.texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Get texture layout info
    D3D12_RESOURCE_DESC textureDesc = handle.texture->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT numRows;
    UINT64 rowSizeInBytes;
    UINT64 totalBytes;
    device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

    // Map upload buffer and copy data
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(handle.uploadBuffer->Map(0, &readRange, &mappedData))) {
        return;
    }

    BYTE* destData = static_cast<BYTE*>(mappedData) + footprint.Offset;
    const BYTE* srcData = rgba.data;
    UINT srcRowPitch = rgba.cols * 4;

    for (UINT row = 0; row < numRows; row++) {
        memcpy(destData + row * footprint.Footprint.RowPitch,
               srcData + row * srcRowPitch,
               srcRowPitch);
    }

    handle.uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = handle.uploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = handle.texture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition texture to shader resource state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = handle.texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    handle.isInShaderResourceState = true;
}

} // namespace gui
