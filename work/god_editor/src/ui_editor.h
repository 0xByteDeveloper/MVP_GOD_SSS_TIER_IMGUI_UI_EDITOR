// ui_editor.h — editör kabuğu (menüler, paneller, başlangıç ekranı, üretim diyaloğu)
#pragma once
#include "imgod.h"
#include "imgui.h"

struct Ctrl;

// editör çapığı durum (editor_main.cpp ile paylaşılır)
struct EditorState {
    int  welcome = 1;              // 1: başlangıç ekranı aktif
    int  editorBackend = 0;       // EditorBackend enum değeri (editörün kendi motoru)
    bool wantQuit = false;
};

namespace ui_editor {

// ana çizim — welcome!=0 iken başlangıç ekranı, değilse editör kabuğu
void Draw(EditorState* es, ImGuiIO& io);

// otomasyon/CLI kancaları
void SetSelftest(const std::string& scriptPath);
void SetPendingOpen(const std::string& projectPath);

// ortak yardımcılar (paneller arası paylaşım)
bool ToolbarButton(const char* label, const char* desc, bool enabled = true);
void StatusUpdate(const std::string& s);

// native dosya diyalogları
std::string OpenDlg(const char* title, const char* filter);
std::string SaveDlg(const char* title, const char* filter, const char* defExt);

// editör küreselleri
extern bool g_showToolbox, g_showCodetree, g_showForms, g_showProps;

} // namespace ui_editor
