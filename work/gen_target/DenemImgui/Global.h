// ============================================================================
// Global.h — IGOD SDK v1 (IMGUI GOD SSS TIER)
// WinForms benzeri sözdizimiyle ImGui nesneleri: button1->Text, slider1->Value
// Tema paletleri (GOD OPS / Midnight Steel / Ammo Tan), Timer, çok formlu runtime.
// ImGui >= 1.90 ile derlenir (ürün 1.91.8 ile test edildi).
// ============================================================================
#pragma once
#include "imgui.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace IGOD {

// —— nesne sarmalayıcıları ---------------------------------------------------
struct Button {
    std::string Text;  ImVec2 SizeArg = ImVec2(0, 0);
    bool Hovered = false, Held = false, Clicked = false;
    Button() {}
    Button(const std::string& t, ImVec2 sz) : Text(t), SizeArg(sz) {}
    bool Draw() {
        Clicked = ImGui::Button(Text.c_str(), SizeArg);
        Hovered = ImGui::IsItemHovered();
        Held = ImGui::IsItemActive();
        return Clicked;
    }
    bool Draw(const ImVec2& at) {
        ImGui::SetCursorPos(at);
        return Draw();
    }
};
struct Checkbox {
    std::string Text;  bool Checked = false;
    bool Checked0 = false;         // tasarım başlangıcı (her karede senkron — editör WYSIWYG)
    Checkbox() {}
    Checkbox(const std::string& t, bool v) : Text(t), Checked(v), Checked0(v) {}
    bool Draw() {
        if (Checked0) { Checked = true; Checked0 = false; }
        bool prev = Checked;
        ImGui::Checkbox(Text.c_str(), &Checked);
        return Checked != prev;
    }
};
struct RadioButton {
    std::string Text;  bool Active = false;
    bool Draw() { return ImGui::RadioButton(Text.c_str(), Active); }
};
struct SliderFloat {
    std::string Text = "Değer";  float Value = 0, Min = 0, Max = 100;
    std::string Format = "%.1f";
    SliderFloat() {}
    SliderFloat(const std::string& t, float v, float mn, float mx) : Text(t), Value(v), Min(mn), Max(mx) {}
    bool Draw() { return ImGui::SliderFloat(Text.c_str(), &Value, Min, Max, Format.c_str()); }
};
struct SliderInt {
    std::string Text = "Değer";  int Value = 0, Min = 0, Max = 100;
    std::string Format = "%d";
    bool Draw() { return ImGui::SliderInt(Text.c_str(), &Value, Min, Max, Format.c_str()); }
};
struct DragFloat {
    std::string Text = "Değer";  float Value = 0, Min = 0, Max = 100, Speed = 1;
    bool Draw() { return ImGui::DragFloat(Text.c_str(), &Value, Speed, Min, Max); }
};
struct DragInt {
    std::string Text = "Değer";  int Value = 0, Min = 0, Max = 100; float Speed = 1;
    bool Draw() { return ImGui::DragInt(Text.c_str(), &Value, Speed, Min, Max); }
};
struct InputText {
    std::string Text = "Yazı";   std::string Value;  std::string Hint;
    bool Password = false;  bool ReadOnly = false;
    char Buf[512] = {};  bool InitBuf = false;  bool EnterPressed = false;
    InputText() {}
    bool Draw() {
        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }
        ImGuiInputTextFlags fl = 0;
        if (Password) fl |= ImGuiInputTextFlags_Password;
        if (ReadOnly) fl |= ImGuiInputTextFlags_ReadOnly;
        bool ch = ImGui::InputText(Text.c_str(), Buf, sizeof Buf, fl);
        if (!Hint.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Hint.c_str());
        EnterPressed = ImGui::IsItemEdited() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsItemDeactivated());
        if (ch) Value = Buf;
        return ch;
    }
};
struct InputTextMultiline {
    std::string Text = "Not";   std::string Value;  bool ReadOnly = false;  ImVec2 Size = ImVec2(280, 120);
    char Buf[4096] = {};  bool InitBuf = false;
    bool Draw() {
        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }
        ImGuiInputTextFlags _fl = ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0;
        bool ch = ImGui::InputTextMultiline(Text.c_str(), Buf, sizeof Buf, Size, _fl);
        if (ch) Value = Buf;
        return ch;
    }
};
struct InputInt {
    std::string Text = "Sayı";  int Value = 0, Step = 1;
    bool Draw() { return ImGui::InputInt(Text.c_str(), &Value, Step); }
};
struct InputFloat {
    std::string Text = "Sayı";  float Value = 0; float Step = 1;
    bool Draw() { return ImGui::InputFloat(Text.c_str(), &Value, Step); }
};
struct Combo {
    std::string Text = "Seçim";  std::vector<std::string> Items;  int Selected = 0;
    Combo() {}
    Combo(const std::vector<std::string>& it) : Items(it) {}
    bool Draw() {
        const char* cur = (Selected >= 0 && Selected < (int)Items.size()) ? Items[Selected].c_str() : "";
        if (ImGui::BeginCombo(Text.c_str(), cur)) {
            for (int i = 0; i < (int)Items.size(); i++) {
                bool sel = (i == Selected);
                if (ImGui::Selectable(Items[i].c_str(), sel) && !sel) { Selected = i; ImGui::EndCombo(); return true; }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return false;
    }
};
struct ListBox {
    std::string Text = "Liste";  std::vector<std::string> Items;  int Selected = 0;  int HeightItems = 5;
    bool Draw() {
        float h = ImGui::GetTextLineHeightWithSpacing() * (HeightItems + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
        if (ImGui::BeginListBox(Text.c_str(), ImVec2(0.0f, h))) {
            for (int i = 0; i < (int)Items.size(); i++) {
                if (ImGui::Selectable(Items[i].c_str(), i == Selected)) Selected = i;
            }
            ImGui::EndListBox();
        }
        return false;
    }
};
struct ColorEdit4 {
    std::string Text = "Renk";  ImVec4 Col = ImVec4(1, 1, 1, 1);
    bool Draw() { return ImGui::ColorEdit4(Text.c_str(), &Col.x); }
};
struct ProgressBar {
    float Value = 0.5f;  std::string Overlay;  ImVec2 Size = ImVec2(200, 18);
    void Draw() {
        char buf[256]; buf[0] = 0;
        if (!Overlay.empty()) snprintf(buf, sizeof buf, "%s", Overlay.c_str());
        ImGui::ProgressBar(Value, Size, buf[0] ? buf : nullptr);
    }
};
struct PlotLines {
    std::string Text = "Grafik";  std::vector<float> Data;  float ScaleMin = 0, ScaleMax = 1; ImVec2 Size = ImVec2(280, 120);
    void Draw() {
        ImGui::PlotLines(Text.c_str(), Data.data(), (int)Data.size(), 0, nullptr, ScaleMin, ScaleMax, Size);
    }
};
struct PlotHistogram {
    std::string Text = "Histogram";  std::vector<float> Data;  ImVec2 Size = ImVec2(280, 120);
    void Draw() { ImGui::PlotHistogram(Text.c_str(), Data.data(), (int)Data.size(), 0, nullptr, FLT_MAX, FLT_MAX, Size); }
};
// konteynerler
struct ChildWindow {
    std::string Text = "Child";  ImVec2 Size = ImVec2(300, 200);  bool Border = true;
    bool Begin() {
        ImGuiChildFlags cf = Border ? ImGuiChildFlags_Borders : 0;
        return ImGui::BeginChild(Text.c_str(), Size, cf);
    }
    void End() { ImGui::EndChild(); }
};
struct GroupBox {
    std::string Text = "Grup";  std::string Id;  ImVec2 Size = ImVec2(320, 160);
    bool Begin() {
        // WinForms GroupBox görünümü: çerçeveli child + üst kenarda başlık
        // Id boşsa Text+"_grp" — editör üretimi benzersiz Id bağlar
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::BeginChild(Id.empty() ? (Text + "_grp").c_str() : (Id + "_grp").c_str(), Size, ImGuiChildFlags_Borders);
        return true;
    }
    void End() {
        // çocuklar kapandı: çerçeve + başlık çiz (GetItemRectMin/Max child'in)
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(rmin, rmax, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
        ImVec2 ts = ImGui::CalcTextSize(Text.c_str());
        ImVec2 lp = ImVec2(rmin.x + 8, rmin.y - ts.y * 0.5f);
        dl->AddRectFilled(ImVec2(lp.x - 4, lp.y - 1), ImVec2(lp.x + ts.x + 4, lp.y + ts.y + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));
        dl->AddText(lp, ImGui::GetColorU32(ImGuiCol_Text), Text.c_str());
    }
};
struct Panel {
    // WinForms Panel karşılığı: kenarlıklı child bölge (bağımsız kaydırılabilir
    // yüzey). Text yalnızca benzersiz child kimliği için ("Görünür##panel1").
    std::string Text = "Panel";  ImVec2 Size = ImVec2(300, 200);
    bool Border = true;  float Rounding = -1.0f;
    bool Begin() {
        if (Rounding >= 0.0f) ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Rounding);
        ImGuiChildFlags cf = Border ? ImGuiChildFlags_Borders : 0;
        return ImGui::BeginChild(Text.c_str(), Size, cf);
    }
    void End() {
        ImGui::EndChild();
        if (Rounding >= 0.0f) ImGui::PopStyleVar();
    }
};
struct CollapsingHeader {
    std::string Text = "Başlık";  bool DefaultOpen = true;
    bool Begin() {
        ImGuiTreeNodeFlags fl = DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::CollapsingHeader(Text.c_str(), fl);
    }
    void End() {}
};
struct TreeNode {
    std::string Text = "Düğüm";  bool DefaultOpen = false;
    bool Begin() {
        ImGuiTreeNodeFlags fl = DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        return ImGui::TreeNodeEx(Text.c_str(), fl);
    }
    void End() { ImGui::TreePop(); }
};
struct TabBar {
    std::string Text = "Sekmeler";
    bool Begin() { return ImGui::BeginTabBar((Text + "_tb").c_str(), ImGuiTabBarFlags_None); }
    void End() { ImGui::EndTabBar(); }
};
struct ModalPopup {
    std::string Text = "Pencere";  bool OpenReq = false;
    void Open() { OpenReq = true; }
    void Close() { OpenReq = false; ImGui::CloseCurrentPopup(); }
    bool Begin() {
        if (OpenReq && !ImGui::IsPopupOpen(Text.c_str()))
            ImGui::OpenPopup(Text.c_str());
        OpenReq = false;
        return ImGui::BeginPopupModal(Text.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    }
    void End() { ImGui::EndPopup(); }
    void Draw() {
        if (Begin()) {
            End();
        }
    }
};
struct Table {
    std::string Text = "Tablo";  int Columns = 3, Rows = 3;
    std::vector<std::string> Headers;
    void Draw() {
        if (ImGui::BeginTable((Text + "_tbl").c_str(), Columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            for (auto& h : Headers) ImGui::TableSetupColumn(h.c_str());
            ImGui::TableHeadersRow();
            for (int r = 0; r < Rows; r++) {
                ImGui::TableNextRow();
                for (int cidx = 0; cidx < Columns; cidx++) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("-");
                }
            }
            ImGui::EndTable();
        }
    }
};
struct MenuBarWrap {
    std::vector<std::string> Menus;                  // "Dosya", "Düzen"...
    std::vector<std::string> Commands;               // "Dosya: Çıkış" biçiminde
    std::string LastCommand;                          // son tıklanan komut (MenuClick'te)
    bool Draw() {
        bool clicked = false;
        if (ImGui::BeginMenuBar()) {
            for (auto& m : Menus) {
                if (ImGui::BeginMenu(m.c_str())) {
                    for (auto& cmd : Commands) {
                        size_t c = cmd.find(':');
                        if (c == std::string::npos) continue;
                        std::string owner = cmd.substr(0, c);
                        std::string item = TrimA2(cmd.substr(c + 1));
                        if (owner != m) continue;
                        if (ImGui::MenuItem(item.c_str())) { LastCommand = m + ":" + item; clicked = true; }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenuBar();
        }
        return clicked;
    }
    static std::string TrimA2(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && (s[a] == ' ')) a++;
        while (b > a && (s[b-1] == ' ')) b--;
        return s.substr(a, b - a);
    }
};
struct FontRef {
    std::string Name = "Arial";  float Size = 18;  bool Bold = false;
    ImFont* Resolve() {
        ImGuiIO& io = ImGui::GetIO();
        ImFontAtlas* atlas = io.Fonts;
        ImFont* f = atlas ? atlas->AddFontFromFileTTF(Name.c_str(), Size) : nullptr;
        return f;
    }
};
// —— bileşenler ---------------------------------------------------------------
struct Timer {
    int Interval = 1000;     // ms
    bool Enabled = false;
    std::function<void()> Tick;
    double accum = 0.0;
    void FrameTick(float dtSec) {
        if (!Enabled || Interval <= 0) return;
        accum += dtSec * 1000.0;
        while (accum >= (double)Interval) {
            accum -= (double)Interval;
            if (Tick) Tick();
        }
    }
};
// —— form temeli + uygulama kütüğü --------------------------------------------
struct FormBase {
    bool Loaded = false;
    bool Visible = false;
    std::vector<Timer*> Timers;
    virtual ~FormBase() {}
    virtual void Render() = 0;
    virtual void OnLoad() {}
    void TickTimers(float dtSec) {
        for (auto* t : Timers) t->FrameTick(dtSec);
    }
    void Show() { Visible = true; Loaded = false; }
    void Hide() { Visible = false; }
};
struct App {
    std::vector<FormBase*> Forms;
    void Register(FormBase* f) { Forms.push_back(f); }
    void Frame(float dtSec) {
        for (auto* f : Forms) {
            if (!f->Visible) continue;
            if (!f->Loaded) { f->OnLoad(); f->Loaded = true; }
            f->TickTimers(dtSec);
            f->Render();
        }
    }
    bool AnyVisible() { for (auto* f : Forms) if (f->Visible) return true; return false; }
};
extern App* g_app;

// —— fontlar: fonts/DejaVuSans.ttf (exe yanı/üstleri) — Türkçe destek ------
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

// —— tema paletleri (üretilen kod ile aynı isimler) ---------------------------
void ApplyGodOpsTheme();
void ApplyMidnightSteelTheme();
void ApplyAmmoTanTheme();

} // namespace IGOD
