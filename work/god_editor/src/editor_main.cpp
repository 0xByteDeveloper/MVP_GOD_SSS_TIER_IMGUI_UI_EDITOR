// ============================================================================
//  editor_main.cpp — IMGUI_GOD_SSSTIER editör girişi
//  * Win32 pencere + seçilebilir grafik motoru (DX11 varsayılan / DX9 / DX10 / OpenGL3)
//  * Başlangıç ekranı: VS seç & bağlan | önceki tasarımı aç | VS'siz boş formla devam
//  * Ana döngü: editör kabuğu (ui_editor) + canvas + paneller
// ============================================================================
#include "imgod.h"
#include "model.h"
#include "forms.h"
#include "theme.h"
#include "vsint.h"
#include "codegen.h"
#include "ui_editor.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_dx10.h"
#include "imgui_impl_opengl3.h"
#include <d3d11.h>
#include <d3d10.h>
#include <d3d9.h>
#include <tchar.h>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "opengl32.lib")

// ---------------------------------------------------------------------------
//  Editör durumları
static EditorState g_es;

// ---- DX11 ----
static ID3D11Device* s_dev11 = nullptr;
static ID3D11DeviceContext* s_ctx11 = nullptr;
static IDXGISwapChain* s_swap11 = nullptr;
static ID3D11RenderTargetView* s_rtv11 = nullptr;
static UINT s_rw11 = 0, s_rh11 = 0;
static void CreateRTV11() {
    ID3D11Texture2D* back = nullptr;
    s_swap11->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { s_dev11->CreateRenderTargetView(back, nullptr, &s_rtv11); back->Release(); }
}
static bool InitDX11(HWND hwnd) {
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
        D3D11_SDK_VERSION, &scd, &s_swap11, &s_dev11, &fl, &s_ctx11);
    if (hr != S_OK)
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, fls, 2,
            D3D11_SDK_VERSION, &scd, &s_swap11, &s_dev11, &fl, &s_ctx11);
    if (hr != S_OK) return false;
    CreateRTV11();
    ImGui_ImplDX11_Init(s_dev11, s_ctx11);
    return true;
}
static void ResizeDX11() {
    if (s_rw11 == 0 || s_rh11 == 0) return;
    ImGui_ImplDX11_InvalidateDeviceObjects();
    if (s_rtv11) { s_rtv11->Release(); s_rtv11 = nullptr; }
    s_swap11->ResizeBuffers(0, s_rw11, s_rh11, DXGI_FORMAT_UNKNOWN, 0);
    s_rw11 = s_rh11 = 0;
    CreateRTV11();
}
static void PresentDX11(HWND hwnd) {
    (void)hwnd;
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    s_swap11->Present(1, 0);
}
static void ShutdownDX11() {
    ImGui_ImplDX11_Shutdown();
    if (s_rtv11) { s_rtv11->Release(); s_rtv11 = nullptr; }
    if (s_swap11) { s_swap11->Release(); s_swap11 = nullptr; }
    if (s_ctx11) { s_ctx11->Release(); s_ctx11 = nullptr; }
    if (s_dev11) { s_dev11->Release(); s_dev11 = nullptr; }
}
// ---- DX9 ----
static LPDIRECT3D9 s_d3d9 = nullptr;
static LPDIRECT3DDEVICE9 s_dev9 = nullptr;
static D3DPRESENT_PARAMETERS s_pp9 = {};
static bool InitDX9(HWND hwnd) {
    if ((s_d3d9 = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr) return false;
    ZeroMemory(&s_pp9, sizeof s_pp9);
    s_pp9.Windowed = TRUE;
    s_pp9.SwapEffect = D3DSWAPEFFECT_DISCARD;
    s_pp9.BackBufferFormat = D3DFMT_UNKNOWN;
    s_pp9.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (s_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &s_pp9, &s_dev9) < 0)
        if (s_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &s_pp9, &s_dev9) < 0)
            return false;
    ImGui_ImplDX9_Init(s_dev9);
    return true;
}
static void PresentDX9(HWND) {
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    HRESULT hr = s_dev9->Present(nullptr, nullptr, nullptr, nullptr);
    if (hr == D3DERR_DEVICELOST) {
        if (s_dev9->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            s_dev9->Reset(&s_pp9);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    }
}
static void ShutdownDX9() {
    ImGui_ImplDX9_Shutdown();
    if (s_dev9) { s_dev9->Release(); s_dev9 = nullptr; }
    if (s_d3d9) { s_d3d9->Release(); s_d3d9 = nullptr; }
}
// ---- DX10 ----
static ID3D10Device* s_dev10 = nullptr;
static IDXGISwapChain* s_swap10 = nullptr;
static ID3D10RenderTargetView* s_rtv10 = nullptr;
static UINT s_rw10 = 0, s_rh10 = 0;
static void CreateRTV10() {
    ID3D10Texture2D* back = nullptr;
    s_swap10->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { s_dev10->CreateRenderTargetView(back, nullptr, &s_rtv10); back->Release(); }
}
static bool InitDX10(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &scd, &s_swap10, &s_dev10) != S_OK) return false;
    CreateRTV10();
    ImGui_ImplDX10_Init(s_dev10);
    return true;
}
static void PresentDX10(HWND) {
    ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    s_swap10->Present(1, 0);
}
static void ShutdownDX10() {
    ImGui_ImplDX10_Shutdown();
    if (s_rtv10) { s_rtv10->Release(); s_rtv10 = nullptr; }
    if (s_swap10) { s_swap10->Release(); s_swap10 = nullptr; }
    if (s_dev10) { s_dev10->Release(); s_dev10 = nullptr; }
}
// ---- OpenGL3 ----
static HDC s_hdc = nullptr; static HGLRC s_hrc = nullptr;
static bool InitGL3(HWND hwnd) {
    s_hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof pfd; pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32;
    int fmt = ChoosePixelFormat(s_hdc, &pfd);
    if (!fmt || !SetPixelFormat(s_hdc, fmt, &pfd)) return false;
    s_hrc = wglCreateContext(s_hdc);
    if (!s_hrc || !wglMakeCurrent(s_hdc, s_hrc)) return false;
    ImGui_ImplOpenGL3_Init();
    return true;
}
static void PresentGL3(HWND hwnd) {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    (void)hwnd;
    SwapBuffers(s_hdc);
}
static void ShutdownGL3() {
    ImGui_ImplOpenGL3_Shutdown();
    wglMakeCurrent(nullptr, nullptr);
    if (s_hrc) { wglDeleteContext(s_hrc); s_hrc = nullptr; }
}

// motor fabrikası
static bool (*InitBE[5])(HWND) = { InitDX11, InitDX9, InitDX10, nullptr, InitGL3 };
static void (*PresentBE[5])(HWND) = { PresentDX11, PresentDX9, PresentDX10, nullptr, PresentGL3 };
static void (*ShutdownBE[5])() = { ShutdownDX11, ShutdownDX9, ShutdownDX10, nullptr, ShutdownGL3 };

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI WndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, msg, w, l)) return true;
    switch (msg) {
    case WM_SIZE:
        if (w == SIZE_MINIMIZED) return 0;
        s_rw11 = LOWORD(l); s_rh11 = HIWORD(l);
        s_rw10 = LOWORD(l); s_rh10 = HIWORD(l);
        return 0;
    case WM_SYSCOMMAND:
        if ((w & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR cmdLine, int nShow) {
    // ---- exe dizini + katalog + form katmanı kurulum ----
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* slash = wcsrchr(exePath, L'\\');
    if (slash) *slash = 0;
    g_exeDirA = WideStrToUtf8(exePath);
    // komut satırı: --backend dx9|dx10|dx11|opengl3 | --selftest <script> | --open <path>
    std::wstring cmd(cmdLine ? cmdLine : L"");
    std::wstring selftestPath;
    std::wstring openPath;
    int backendSel = (int)EditorBackend::DX11;
    {
        std::vector<std::wstring> toks;
        std::wstring cur;
        for (wchar_t ch : std::wstring(cmdLine ? cmdLine : L"")) {
            if (ch == L' ') { if (!cur.empty()) { toks.push_back(cur); cur.clear(); } }
            else cur += ch;
        }
        if (!cur.empty()) toks.push_back(cur);
        for (size_t i = 0; i < toks.size(); i++) {
            if (toks[i] == L"--backend" && i + 1 < toks.size()) {
                std::wstring b = toks[++i];
                if (b == L"dx9") backendSel = (int)EditorBackend::DX9;
                else if (b == L"dx10") backendSel = (int)EditorBackend::DX10;
                else if (b == L"opengl3") backendSel = (int)EditorBackend::OPENGL3;
                else if (b == L"dx12") backendSel = (int)EditorBackend::DX12;
            } else if (toks[i] == L"--selftest" && i + 1 < toks.size()) selftestPath = toks[++i];
            else if (toks[i] == L"--open" && i + 1 < toks.size()) openPath = toks[++i];
        }
    }
    if (backendSel == (int)EditorBackend::DX12) backendSel = (int)EditorBackend::DX11;  // editör kendisi dx11'e düşer (dx12 üretim içindir)
    g_es.editorBackend = backendSel;

    g_catalog.LoadBuiltin();
    forms::Init();

    // ---- pencere ----
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGUI_GOD_SSSTIER", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — UI Editör ve Kod Üreticisi",
        WS_OVERLAPPEDWINDOW, 60, 40, 1680, 960, nullptr, nullptr, wc.hInstance, nullptr);
    if (!InitBE[g_es.editorBackend](hwnd)) {
        // motor açılamadı → DX11'e dön
        ShutdownBE[g_es.editorBackend]();
        g_es.editorBackend = (int)EditorBackend::DX11;
        if (!InitBE[g_es.editorBackend](hwnd)) {
            MessageBoxW(hwnd, L"Grafik motoru başlatılamadı (DX11 dahil).", L"Hata", MB_OK | MB_ICONERROR);
            return 1;
        }
    }
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;   // pencere konum kaydı yok — editör düzeni sabit
    ImGui_ImplWin32_Init(hwnd);

    theme::Apply(theme::TImGuiDark);

    if (!selftestPath.empty()) ui_editor::SetSelftest(WideStrToUtf8(selftestPath));
    if (!openPath.empty()) ui_editor::SetPendingOpen(WideStrToUtf8(openPath));
    if (!selftestPath.empty()) g_es.welcome = 0;   // otomatik modda başlangıç ekranı atlanır

    // ---- ana döngü ----
    while (!g_es.wantQuit) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) { g_es.wantQuit = true; break; }
        }
        if (g_es.wantQuit) break;
        if (g_es.editorBackend == (int)EditorBackend::DX11) ResizeDX11();
        if (g_es.editorBackend == (int)EditorBackend::DX10 && s_rw10) {
            ImGui_ImplDX10_InvalidateDeviceObjects();
            if (s_rtv10) { s_rtv10->Release(); s_rtv10 = nullptr; }
            s_swap10->ResizeBuffers(0, s_rw10, s_rh10, DXGI_FORMAT_UNKNOWN, 0);
            s_rw10 = s_rh10 = 0;
            CreateRTV10();
        }
        switch (g_es.editorBackend) {
        case (int)EditorBackend::DX11: ImGui_ImplDX11_NewFrame(); break;
        case (int)EditorBackend::DX9:  ImGui_ImplDX9_NewFrame(); break;
        case (int)EditorBackend::DX10: ImGui_ImplDX10_NewFrame(); break;
        case (int)EditorBackend::OPENGL3: ImGui_ImplOpenGL3_NewFrame(); break;
        }
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // editör kabuğu (başlangıç ekranı dahil)
        ui_editor::Draw(g_es.welcome ? nullptr : &g_es, io);

        ImGui::Render();
        const float clear[] = { 0.07f, 0.075f, 0.10f, 1.00f };
        switch (g_es.editorBackend) {
        case (int)EditorBackend::DX11:
            s_ctx11->OMSetRenderTargets(1, &s_rtv11, nullptr);
            s_ctx11->ClearRenderTargetView(s_rtv11, clear);
            break;
        case (int)EditorBackend::DX9:
            s_dev9->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(18, 19, 26), 1.0f, 0);
            break;
        case (int)EditorBackend::DX10:
            s_dev10->OMSetRenderTargets(1, &s_rtv10, nullptr);
            s_dev10->ClearRenderTargetView(s_rtv10, clear);
            break;
        case (int)EditorBackend::OPENGL3:
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(0.07f, 0.075f, 0.10f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            break;
        }
        PresentBE[g_es.editorBackend](hwnd);
    }

    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ShutdownBE[g_es.editorBackend]();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
