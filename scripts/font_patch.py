#!/usr/bin/env python3
# font_patch.py — Türkçe font altyapısı: editör + üretilen projeler
# 1) editor_main.cpp: ProbeFont + SetupFonts (v1'den port)
# 2) codegen Global.h: IGOD::SetupFonts + ProbeFontA
# 3) codegen main şablonları: CreateContext sonrası IGOD::SetupFonts();
# 4) codegen EmitProject: fonts/ kopyalama

BS = "\\"

# ---------- 1) editor_main.cpp ----------
p = "/home/z/my-project/work/god_editor/src/editor_main.cpp"
s = open(p, encoding="utf-8").read()

fonts_fn = r'''
// ---------------------------------------------------------------------------
//  fontlar: fonts/DejaVuSans.ttf (exe yanı / üst klasörler) — Türkçe destek
//  Varsayılan ProggyClean yalnız ASCII'dir; ğ ş ı İ ö ü ç için TTF şart.
// ---------------------------------------------------------------------------
static std::string ProbeFontA(const char* rel) {
    char exeDir[MAX_PATH];
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    char* slash = strrchr(exeDir, '\\');
    if (slash) *slash = 0;
    const char* affixes[] = { "", "\\..", "\\..\\..", "\\..\\..\\.." };
    for (auto& a : affixes) {
        std::string pth = std::string(exeDir) + a + "\\" + rel;
        if (GetFileAttributesA(pth.c_str()) != INVALID_FILE_ATTRIBUTES) return pth;
    }
    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        std::string pth = std::string(cwd) + "\\" + rel;
        if (GetFileAttributesA(pth.c_str()) != INVALID_FILE_ATTRIBUTES) return pth;
    }
    return "";
}
static void SetupFonts() {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = { 0x0020, 0x01FF, 0x2010, 0x205F, 0x2190, 0x21FF, 0 };  // Latin-1 + Ext-A (ğ ş ı İ) + oklar
    std::string sans = ProbeFontA("fonts\\DejaVuSans.ttf");
    if (!sans.empty()) io.Fonts->AddFontFromFileTTF(sans.c_str(), 17.0f, nullptr, ranges);
    else io.Fonts->AddFontDefault();
    std::string mono = ProbeFontA("fonts\\DejaVuSansMono.ttf");
    if (!mono.empty()) io.FontDefault = io.Fonts->AddFontFromFileTTF(mono.c_str(), 15.0f, nullptr, ranges);
    io.Fonts->Build();
}
'''
if "SetupFonts()" not in s.split("int WINAPI")[0]:
    # wWinMain'den önce ekle (extern LRESULT satırından önce)
    anchor = "extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);"
    assert anchor in s
    s = s.replace(anchor, fonts_fn + "\n" + anchor, 1)

# CreateContext'ten sonra çağır (backend init'ten ÖNCE — font atlası hazır olmalı)
old_cc = '''    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;'''
new_cc = '''    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetupFonts();                     // DejaVu — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;'''
assert old_cc in s
s = s.replace(old_cc, new_cc, 1)
open(p, "w", encoding="utf-8").write(s)
print("editor_main fonts OK")

# ---------- 2) codegen.cpp ----------
p = "/home/z/my-project/work/god_editor/src/codegen.cpp"
s = open(p, encoding="utf-8").read()

# Global.h şablonuna SetupFonts ekle (tema paletlerinden önce)
gh_anchor = "// —— tema paletleri (üretilen kod ile aynı isimler) ---------------------------"
gh_fonts = r'''// —— fontlar: fonts/DejaVuSans.ttf (exe yanı/üstleri) — Türkçe destek ------
inline std::string ProbeFontA(const char* rel) {
    char exeDir[MAX_PATH];
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    char* slash = strrchr(exeDir, '\\');
    if (slash) *slash = 0;
    const char* affixes[] = { "", "\\..", "\\..\\..", "\\..\\..\\.." };
    for (auto& a : affixes) {
        std::string p = std::string(exeDir) + a + "\\" + rel;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        std::string p = std::string(cwd) + "\\" + rel;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    return "";
}
inline void SetupFonts() {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = { 0x0020, 0x01FF, 0x2010, 0x205F, 0x2190, 0x21FF, 0 };  // Latin-1 + Ext-A + oklar
    std::string sans = ProbeFontA("fonts\\DejaVuSans.ttf");
    if (!sans.empty()) io.Fonts->AddFontFromFileTTF(sans.c_str(), 17.0f, nullptr, ranges);
    else io.Fonts->AddFontDefault();
    std::string mono = ProbeFontA("fonts\\DejaVuSansMono.ttf");
    if (!mono.empty()) io.FontDefault = io.Fonts->AddFontFromFileTTF(mono.c_str(), 15.0f, nullptr, ranges);
    io.Fonts->Build();
}

'''
# Global.h raw string içinde mi? anchor'ı raw içinde ara (R"IGODSDK(...")
if gh_anchor in s:
    s = s.replace(gh_anchor, gh_fonts + gh_anchor, 1)
else:
    raise SystemExit("Global.h font anchor yok")

# Global.h başlığına windows.h ekle (MAX_PATH/GetModuleFileNameA için)
s = s.replace('#pragma once\n#include "imgui.h"',
              '#pragma once\n#include "imgui.h"\n#include <windows.h>\n#include <cstring>', 1)

# main.cpp şablonları: CreateContext sonrası IGOD::SetupFonts();
n = s.count("    ImGui::CreateContext();")
s = s.replace("    ImGui::CreateContext();", "    ImGui::CreateContext();\n    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği")
print(f"main templates patched: {n}")

# EmitProject: fonts kopyalama — CopyImGuiSdk çağrısından sonra
fonts_copy = r'''    // fonts/ kopyala (DejaVu — Türkçe karakter desteği; editör exe yanı/üstlerinden)
    {
        extern std::string g_exeDirA;
        const char* cand[] = { "fonts/DejaVuSans.ttf", "fonts\\DejaVuSans.ttf" };
        std::string srcFont;
        // exe yanında ve üstlerinde ara
        std::vector<std::string> roots{ g_exeDirA };
        for (int up = 0; up < 3; up++) {
            std::string r = roots.back();
            size_t slash = r.find_last_of("/\\");
            if (slash == std::string::npos) break;
            roots.push_back(r.substr(0, slash));
        }
        roots.push_back(".");
        for (auto& r : roots) {
            std::string f = JoinPathA(r, "fonts/DejaVuSans.ttf");
            std::string data;
            if (ReadFileTextA(f, data)) { srcFont = r; break; }
        }
        if (!srcFont.empty()) {
            std::string dstFonts = JoinPathA(projDir, "fonts");
            MkDirsA(dstFonts);
            const char* names[] = { "DejaVuSans.ttf", "DejaVuSansMono.ttf" };
            for (auto& n2 : names) {
                std::string data;
                if (ReadFileTextA(JoinPathA(srcFont, std::string("fonts/") + n2), data))
                    WriteFileTextA(JoinPathA(dstFonts, n2), data);
            }
        }
    }
'''
anchor2 = '''    // ImGui SDK kopyası
    if (!CopyImGuiSdk(projDir)) return false;'''
assert anchor2 in s
s = s.replace(anchor2, anchor2 + "\n" + fonts_copy, 1)
open(p, "w", encoding="utf-8").write(s)
print("codegen fonts OK")
