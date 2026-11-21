// ============================================================================
//  imgod.h — IMGUI_GOD_SSSTIER ortak tanımlar ve yardımcılar
//  M2GodWin32 Faz 2.13'ün ImGui tabanlı GOD SSS TIER dönüşümü.
//  Visual Studio 2019 C++CLR Form Designer deneyimini ImGui ile yaşatan
//  tam kapsamlı görsel UI tasarım ortamı + kod üreticisi.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <functional>
#include <cwctype>
#include <cstdio>
#include <cstdint>

// ---------------------------------------------------------------------------
//  Ölçü sabitleri (özgün M2GodWin32 düzeni korundu — RAD Studio tarzı)
// ---------------------------------------------------------------------------
constexpr int TOOLBOX_W  = 225;    // sol alt: araç kutusu genişliği
constexpr int CODETREE_W = 260;    // sol üst: kod gezgini genişliği
constexpr int FORMLIST_H = 150;    // sağ üst: formlar listesi yüksekliği
constexpr int PROPS_W    = 356;    // sağ: özellikler paneli genişliği
constexpr int MENUBAR_H  = 20;
constexpr int STATUSBAR_H = 24;
constexpr int SPLIT_W    = 6;     // sürüklenebilir ayraç kalınlığı
constexpr int GRID_SIZE  = 8;      // tasarım ızgarası (snap)
constexpr int SEL_MARGIN = 4;      // seçim tutamacı payı

// ---------------------------------------------------------------------------
//  Dize yardımcıları (taşınabilir, Win32 CRT bağımlılığı yok)
// ---------------------------------------------------------------------------
std::wstring Utf8ToWideStr(const std::string& s);
std::string  WideStrToUtf8(const std::wstring& w);
std::wstring TrimW(const std::wstring& s);
std::wstring LowerW(std::wstring s);
std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep);
std::wstring JoinW(const std::vector<std::wstring>& v, wchar_t sep);
std::wstring ReplaceAllW(std::wstring s, const std::wstring& from, const std::wstring& to);
bool EqCIW(const std::wstring& a, const std::wstring& b);
std::string TrimA(std::string s);

// dosya yardımcıları
bool  ReadFileTextA(const std::string& path, std::string& out);
bool  WriteFileTextA(const std::string& path, const std::string& data);
bool  FileExistsA(const std::string& path);
bool  MkDirsA(const std::string& path);                 // özyinelemeli mkdir
std::string DirOfA(const std::string& path);           // klasör (son / yok)
std::string JoinPathA(const std::string& a, const std::string& b);
std::string BaseNameA(const std::string& path);
std::vector<std::string> ListFilesA(const std::string& dir, const std::string& ext);

// ---------------------------------------------------------------------------
//  Metin kaçışları (kod üretimi için)
// ---------------------------------------------------------------------------
std::string EscCppUtf8(const std::string& s);   // " \ \n \r \t → C++ sabiti (utf-8 u8"" içinde)
std::string EscIdent(const std::string& s);     // [A-Za-z0-9_] dışını _ yapar

// ---------------------------------------------------------------------------
//  Uygulama çapında durum
// ---------------------------------------------------------------------------
enum class EditorBackend {
    DX11 = 0, DX9 = 1, DX10 = 2, DX12 = 3, OPENGL3 = 4
};
const char* BackendNameA(EditorBackend b);      // "DirectX 11" ...
const char* BackendIdA(EditorBackend b);        // "dx11"/"dx9"/"dx10"/"dx12"/"opengl3"

extern bool g_modelDirty;
extern bool g_gridVisible;
extern bool g_snapEnabled;
extern std::string g_statusText;
extern std::string g_exeDirA;
extern void* g_mainHwnd;      // editör ana pencere (HWND — selftest tıklama enjeksiyonu)
