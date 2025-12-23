// ArUco Markers GUI - Main Application
// Based on Dear ImGui DirectX 11 example

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
#include <imgui_impl_dx11.h>

#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

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
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ArUco GUI", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ArUco Markers GUI", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
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
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize texture manager and video thread
    g_textureManager = std::make_unique<gui::TextureManager>(g_pd3dDevice, g_pd3dDeviceContext);
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
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
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
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup panels
    if (g_activePanel >= 0 && g_activePanel < static_cast<int>(g_panels.size())) {
        g_panels[g_activePanel]->onDeactivate();
    }
    g_panels.clear();
    g_videoThread.reset();
    g_textureManager.reset();

    // Cleanup ImGui
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
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
