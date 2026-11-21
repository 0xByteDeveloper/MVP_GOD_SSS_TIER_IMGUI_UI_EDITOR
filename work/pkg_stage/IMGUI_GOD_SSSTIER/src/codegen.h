// ============================================================================
//  codegen.h — ImGui C++ kod üreticisi (projenin KALBİ)
//  Editördeki tasarımı → derlenince AYNEN o formu gösteren ImGui projesine
//  çevirir:
//    * VS2019 çözümü (.sln + .vcxproj v142) VEYA MinGW derleme betiği
//    * Seçilen D3D/Grafik backend'i (dx9/dx10/dx11/dx12/opengl3)
//    * Her form <Ad>.h/.cpp — Global.h (IGOD SDK) ile WinForms benzeri sözdizimi:
//        button1->Text = u8"Merhaba";  slider1->Value = 90.0f;
//  Üretilen kod, ImGui 1.76 → 1.93 arasında AYNI widget API'leriyle uyumlu.
// ============================================================================
#pragma once
#include "model.h"
#include <string>

namespace codegen {

// Üretim ayarları (D3D sürüm seçimi dahil — kullanıcı talebi)
struct GenSettings {
    int  backend = 0;              // EditorBackend enum değeri (0=dx11)
    int  themeId = 0;              // theme::ThemeId
    bool genVS2019 = true;         // VS2019 çözümü üret
    bool genMinGW = true;          // build_mingw.sh da üret
    bool x64 = true;               // vcxproj platformu
    std::string projectName = "IMGodProject";
    std::string imguiDir = "imgui";  // editör exe yanındaki ImGui SDK klasörü
};
extern GenSettings g_settings;

// Aktif çok-formlu projeyi klasöre yaz:
//   <folder>/
//     <Project>.sln, <Project>/<Project>.vcxproj (v142, x64/Win32, C++14)
//     <Project>/imgui/... (kaynak kopyası)
//     <Project>/main.cpp, Global.h, Form1.cpp/h, Form2.cpp/h ...
//     build_mingw.sh (bağımsız derleme — MinGW/clang)
bool EmitProject(const std::string& folder);

// tek form için <Ad>.cpp gövdesi (testler için de kullanılır)
std::string EmitFormCpp(const std::string& formName, const Model& m);
std::string EmitFormH(const std::string& formName, const Model& m);
std::string EmitGlobalH();          // IGOD SDK (wrapper + tema + timer + form runtime)
std::string EmitMainCpp();          // wWinMain + backend init + form döngüsü
std::string EmitSln(const std::string& projectName);
std::string EmitVcxproj(const std::string& projectName);
std::string EmitMingwScript(const std::string& projectName);

// ImGui SDK dosyalarını hedefe kopyalar (imgui + seçili backend + win32)
bool CopyImGuiSdk(const std::string& destRoot);

// handler adı: "button1" + "Click" → "button1_Click"
std::string HandlerOf(const std::string& obj, const std::string& ev);

} // namespace codegen
