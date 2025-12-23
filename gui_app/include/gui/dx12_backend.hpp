#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace gui {

using Microsoft::WRL::ComPtr;

class DX12Backend {
public:
    static constexpr UINT NUM_BACK_BUFFERS = 3;
    static constexpr UINT NUM_SRV_DESCRIPTORS = 256;

    DX12Backend();
    ~DX12Backend();

    bool initialize(HWND hwnd, int width, int height);
    void shutdown();
    void resize(int width, int height);

    void beginFrame();
    void endFrame();
    void present();

    ID3D12Device* getDevice() const { return device_.Get(); }
    ID3D12DescriptorHeap* getSrvHeap() const { return srvHeap_.Get(); }
    ID3D12GraphicsCommandList* getCommandList() const { return commandList_.Get(); }

    UINT getSrvDescriptorSize() const { return srvDescriptorSize_; }
    UINT getNextSrvIndex() { return nextSrvIndex_++; }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;

    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ComPtr<ID3D12DescriptorHeap> srvHeap_;
    ComPtr<ID3D12CommandAllocator> commandAllocators_[NUM_BACK_BUFFERS];
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12Resource> renderTargets_[NUM_BACK_BUFFERS];

    UINT64 fenceValues_[NUM_BACK_BUFFERS] = {};
    HANDLE fenceEvent_ = nullptr;
    UINT frameIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;
    UINT srvDescriptorSize_ = 0;
    UINT nextSrvIndex_ = 1;  // 0 is reserved for ImGui font

    bool createDevice();
    bool createCommandQueue();
    bool createSwapChain(int width, int height);
    bool createRenderTargets();
    bool createDescriptorHeaps();
    bool createCommandAllocatorsAndList();
    bool createFence();

    void waitForGpu();
    void moveToNextFrame();
};

} // namespace gui
