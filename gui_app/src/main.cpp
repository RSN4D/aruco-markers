// ArUco Markers GUI - Main Application
// Based on Dear ImGui DirectX 12 example

#include "gui/dx12_backend.hpp"
#include "gui/texture_manager.hpp"
#include "gui/video_thread.hpp"
#include "gui/panels/panel_base.hpp"
#include "gui/panels/marker_panel.hpp"
#include "gui/panels/board_panel.hpp"
#include "gui/panels/detect_panel.hpp"
#include "gui/panels/calibration_panel.hpp"
#include "gui/panels/pose_panel.hpp"
#include "gui/panels/cube_panel.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

// Data
static std::unique_ptr<gui::DX12Backend> g_backend;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;

// Application state
static std::unique_ptr<gui::TextureManager> g_textureManager;
static std::unique_ptr<gui::VideoThread> g_videoThread;
static std::vector<std::unique_ptr<gui::PanelBase>> g_panels;
static int g_activePanel = 0;
static int g_previousPanel = -1;
static int g_pendingTabSelection = -1;  // For programmatic tab changes (e.g., from menu)
static double g_fps = 0.0;
static int g_frameWidth = 0;
static int g_frameHeight = 0;
static std::string g_statusMessage = "Ready";

// Forward declarations
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ArUco GUI", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ArUco Markers GUI (DX12)", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D 12
    g_backend = std::make_unique<gui::DX12Backend>();
    if (!g_backend->initialize(hwnd, 1280, 800))
    {
        g_backend.reset();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.GrabRounding = 4.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    // Get font descriptor handle for ImGui
    ID3D12DescriptorHeap* srvHeap = g_backend->getSrvHeap();
    D3D12_CPU_DESCRIPTOR_HANDLE fontCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE fontGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

    ImGui_ImplDX12_Init(g_backend->getDevice(), gui::DX12Backend::NUM_BACK_BUFFERS,
                        DXGI_FORMAT_R8G8B8A8_UNORM, srvHeap,
                        fontCpuHandle, fontGpuHandle);

    // Initialize texture manager and video thread
    g_textureManager = std::make_unique<gui::TextureManager>(g_backend.get());
    g_videoThread = std::make_unique<gui::VideoThread>();

    // Create panels
    g_panels.push_back(std::make_unique<gui::MarkerPanel>(*g_textureManager, *g_videoThread));
    g_panels.push_back(std::make_unique<gui::BoardPanel>(*g_textureManager, *g_videoThread));
    g_panels.push_back(std::make_unique<gui::DetectPanel>(*g_textureManager, *g_videoThread));
    g_panels.push_back(std::make_unique<gui::CalibrationPanel>(*g_textureManager, *g_videoThread));
    g_panels.push_back(std::make_unique<gui::PosePanel>(*g_textureManager, *g_videoThread));
    g_panels.push_back(std::make_unique<gui::CubePanel>(*g_textureManager, *g_videoThread));

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_backend->resize(g_ResizeWidth, g_ResizeHeight);
            g_ResizeWidth = g_ResizeHeight = 0;
        }

        // Start frame
        g_backend->beginFrame();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main window - fill entire viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_MenuBar;

        ImGui::Begin("MainWindow", nullptr, windowFlags);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    done = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                for (int i = 0; i < static_cast<int>(g_panels.size()); i++) {
                    bool selected = (g_activePanel == i);
                    if (ImGui::MenuItem(g_panels[i]->getName().c_str(), nullptr, selected)) {
                        g_pendingTabSelection = i;  // Request tab change via menu
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    // Could show about dialog
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Panel tabs
        if (ImGui::BeginTabBar("PanelTabs")) {
            for (int i = 0; i < static_cast<int>(g_panels.size()); i++) {
                ImGuiTabItemFlags flags = 0;
                // Only use SetSelected for programmatic changes (e.g., from menu)
                if (g_pendingTabSelection == i) {
                    flags |= ImGuiTabItemFlags_SetSelected;
                }

                if (ImGui::BeginTabItem(g_panels[i]->getName().c_str(), nullptr, flags)) {
                    g_activePanel = i;
                    ImGui::EndTabItem();
                }
            }
            // Clear pending selection after processing all tabs
            g_pendingTabSelection = -1;
            ImGui::EndTabBar();
        }

        ImGui::Separator();

        // Render active panel
        if (g_activePanel >= 0 && g_activePanel < static_cast<int>(g_panels.size())) {
            // Handle panel activation/deactivation
            if (g_previousPanel != g_activePanel) {
                if (g_previousPanel >= 0 && g_previousPanel < static_cast<int>(g_panels.size())) {
                    g_panels[g_previousPanel]->onDeactivate();
                }
                g_panels[g_activePanel]->onActivate();
                g_previousPanel = g_activePanel;
            }

            g_panels[g_activePanel]->render();
        }

        // Status bar
        if (g_videoThread->isRunning()) {
            cv::Size frameSize = g_videoThread->getFrameSize();
            g_frameWidth = frameSize.width;
            g_frameHeight = frameSize.height;
            g_fps = g_videoThread->getFPS();
        }

        ImGui::Separator();

        ImGui::Text("Status: %s", g_statusMessage.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 300);

        if (g_fps > 0) {
            ImGui::Text("FPS: %.1f", g_fps);
            ImGui::SameLine();
        }

        if (g_frameWidth > 0 && g_frameHeight > 0) {
            ImGui::Text("Resolution: %dx%d", g_frameWidth, g_frameHeight);
        }

        ImGui::End();

        // Rendering
        ImGui::Render();

        ID3D12GraphicsCommandList* cmdList = g_backend->getCommandList();
        cmdList->SetDescriptorHeaps(1, &srvHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

        g_backend->endFrame();
        g_backend->present();
    }

    // Cleanup panels
    if (g_activePanel >= 0 && g_activePanel < static_cast<int>(g_panels.size())) {
        g_panels[g_activePanel]->onDeactivate();
    }
    g_panels.clear();
    g_videoThread.reset();
    g_textureManager.reset();

    // Cleanup ImGui
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_backend->shutdown();
    g_backend.reset();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
