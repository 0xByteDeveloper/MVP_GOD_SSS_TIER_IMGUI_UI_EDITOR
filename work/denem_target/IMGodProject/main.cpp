// main.cpp — IMGUI_GOD_SSSTIER üretimi (DirectX 11)
#include "Global.h"
#include "Form1.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static UINT g_rw = 0, g_rh = 0;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRTV() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void CleanupRTV() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }
static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL fls[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, fls, 2,
        D3D11_SDK_VERSION, &scd, &g_swap, &g_dev, &fl, &g_ctx);
    if (hr != S_OK)
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, fls, 2,
            D3D11_SDK_VERSION, &scd, &g_swap, &g_dev, &fl, &g_ctx);
    if (hr != S_OK) return false;
    CreateRTV();
    return true;
}
static void CleanupDevice() {
    CleanupRTV();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_rw = LOWORD(lParam); g_rh = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
IGOD::App* IGOD::g_app = nullptr;   // Global.h extern tanımı (file scope)
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGodApp", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — Uygulama", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDevice(hwnd)) { MessageBoxW(hwnd, L"D3D11 cihazı oluşturulamadı.", L"Hata", MB_OK | MB_ICONERROR); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    IGOD::App app;
    IGOD::g_app = &app;
    Form1 form1;             // başlangıç formu (listeden ilk)
    app.Register(&form1);
    form1.Show();

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (g_rw != 0 && g_rh != 0) {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            CleanupRTV();
            g_swap->ResizeBuffers(0, g_rw, g_rh, DXGI_FORMAT_UNKNOWN, 0);
            g_rw = g_rh = 0;
            CreateRTV();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!app.AnyVisible()) PostQuitMessage(0);
        app.Frame(io.DeltaTime);
        ImGui::Render();
        const float clear[] = { 0.08f, 0.09f, 0.12f, 1.00f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
