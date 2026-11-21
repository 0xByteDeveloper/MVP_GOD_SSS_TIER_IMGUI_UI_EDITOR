// ============================================================================
//  objstyle.cpp — nesne stili koleksiyonu (canvas + codegen ortak kalbi).
//  Renk prop'ları "#RRGGBB"; boş/Legacy → kalıtım (push edilmez).
//  Stil değişkenleri: FrameRounding / FrameBorderSize (negatif = kalıtım),
//  Alpha (0..1, 1 = kalıtım), Disabled (BeginDisabled ile sarılır).
// ============================================================================
#include "objstyle.h"
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace objstyle {

// ---------------------------------------------------------------------------
//  "#RRGGBB" / "#RRGGBBAA" ayrıştırma
bool ParseHexColor(const std::string& s, float out[4]) {
    if (s.empty()) return false;
    const char* p = s.c_str();
    if (*p == '#') p++;
    size_t len = strlen(p);
    if (len != 6 && len != 8) return false;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    int r = nib(p[0]) * 16 + nib(p[1]);
    int g = nib(p[2]) * 16 + nib(p[3]);
    int b = nib(p[4]) * 16 + nib(p[5]);
    int a = (len == 8) ? (nib(p[6]) * 16 + nib(p[7])) : 255;
    if (r < 0 || g < 0 || b < 0 || a < 0) return false;
    out[0] = r / 255.0f; out[1] = g / 255.0f; out[2] = b / 255.0f; out[3] = a / 255.0f;
    return true;
}
std::string HexOf(const float rgba[4]) {
    int r = (int)(rgba[0] * 255.0f + 0.5f), g = (int)(rgba[1] * 255.0f + 0.5f);
    int b = (int)(rgba[2] * 255.0f + 0.5f), a = (int)(rgba[3] * 255.0f + 0.5f);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    if (a < 0) a = 0; if (a > 255) a = 255;
    char buf[16];
    if (a >= 255) snprintf(buf, sizeof buf, "#%02X%02X%02X", r, g, b);
    else snprintf(buf, sizeof buf, "#%02X%02X%02X%02X", r, g, b, a);
    return buf;
}

// ---------------------------------------------------------------------------
//  prop → stil girişi eşlemesi (tip bazlı)
static void AddColor(std::vector<Entry>& out, const Ctrl* c, const char* prop, ImGuiCol_ colId) {
    std::string v = c->Get(prop, "");
    float f[4];
    if (!ParseHexColor(v, f)) return;
    Entry e;
    e.isColor = true;
    e.id = (int)colId;
    e.v0 = f[0]; e.v1 = f[1]; e.v2 = f[2]; e.v3 = f[3];
    e.propName = prop;
    out.push_back(e);
}

std::vector<Entry> Collect(const Ctrl* c) {
    std::vector<Entry> out;
    const std::string& t = c->type;

    // — genel: metin rengi
    AddColor(out, c, "TextColor", ImGuiCol_Text);

    // — düğme ailesi
    if (t == "Button") {
        AddColor(out, c, "ButtonColor", ImGuiCol_Button);
        AddColor(out, c, "ButtonHoverColor", ImGuiCol_ButtonHovered);
        AddColor(out, c, "ButtonActiveColor", ImGuiCol_ButtonActive);
    }
    // — çerçeve ailesi (giriş/kaydırma/seçim)
    if (t == "Checkbox" || t == "RadioButton" || t == "SliderFloat" || t == "SliderInt" ||
        t == "DragFloat" || t == "DragInt" || t == "InputText" || t == "InputTextMultiline" ||
        t == "InputInt" || t == "InputFloat" || t == "Combo" || t == "ListBox" ||
        t == "ColorEdit4" || t == "ProgressBar" || t == "Button") {
        AddColor(out, c, "FrameBgColor", ImGuiCol_FrameBg);
        AddColor(out, c, "FrameHoverColor", ImGuiCol_FrameBgHovered);
        AddColor(out, c, "FrameActiveColor", ImGuiCol_FrameBgActive);
    }
    if (t == "Checkbox" || t == "RadioButton") AddColor(out, c, "CheckMarkColor", ImGuiCol_CheckMark);
    if (t == "SliderFloat" || t == "SliderInt") {
        AddColor(out, c, "SliderGrabColor", ImGuiCol_SliderGrab);
        AddColor(out, c, "SliderGrabActiveColor", ImGuiCol_SliderGrabActive);
    }
    if (t == "ProgressBar") AddColor(out, c, "ProgressColor", ImGuiCol_PlotHistogram);
    if (t == "PlotLines")   AddColor(out, c, "PlotColor", ImGuiCol_PlotLines);
    if (t == "PlotHistogram") AddColor(out, c, "PlotColor", ImGuiCol_PlotHistogram);
    // — konteynerler (Panel dahil)
    if (t == "Panel" || t == "ChildWindow" || t == "GroupBox" || t == "CollapsingHeader" || t == "TreeNode") {
        AddColor(out, c, "BgColor", ImGuiCol_ChildBg);
        AddColor(out, c, "BorderColor", ImGuiCol_Border);
    }
    if (t == "TabBar") AddColor(out, c, "TabSelectedColor", ImGuiCol_TabSelected);
    if (t == "Table")  AddColor(out, c, "HeaderBgColor", ImGuiCol_TableHeaderBg);
    if (t == "Separator") AddColor(out, c, "SeparatorColor", ImGuiCol_Separator);
    if (t == "SeparatorText") AddColor(out, c, "SeparatorTextColor", ImGuiCol_TextDisabled);

    // — stil değişkenleri
    // Panel'de "Rounding" → ChildRounding (child bölge köşesi); diğerlerinde
    // FrameRounding (çerçeve köşesi). Canvas ve üretilen kod AYNI girişi kullanır.
    {
        const bool isPanel = (t == "Panel");
        const char* rndProp = isPanel ? "Rounding" : "FrameRounding";
        float fr = (float)atof(c->Get(rndProp, "").c_str());
        if (c->props.count(rndProp) && fr >= 0.0f) {
            Entry e; e.id = (int)(isPanel ? ImGuiStyleVar_ChildRounding : ImGuiStyleVar_FrameRounding);
            e.v0 = fr; e.propName = rndProp;
            out.push_back(e);
        }
    }
    float fb = (float)atof(c->Get("FrameBorderSize", "").c_str());
    if (c->props.count("FrameBorderSize") && fb >= 0.0f) {
        Entry e; e.id = (int)ImGuiStyleVar_FrameBorderSize; e.v0 = fb; e.propName = "FrameBorderSize";
        out.push_back(e);
    }
    float alpha = (float)atof(c->Get("Alpha", "1").c_str());
    if (c->props.count("Alpha") && alpha > 0.0f && alpha < 1.0f) {
        Entry e; e.id = (int)ImGuiStyleVar_Alpha; e.v0 = alpha; e.propName = "Alpha";
        out.push_back(e);
    }
    return out;
}

// ---------------------------------------------------------------------------
//  kod üretimi sayacı (ImGui çağrısı YAPMAZ)
int CountPushes(const Ctrl* c) {
    int nCol = 0, nVar = 0;
    for (auto& e : Collect(c)) { if (e.isColor) nCol++; else nVar++; }
    return nCol * 1000 + nVar;
}

// ---------------------------------------------------------------------------
//  canvas uygulaması
int Push(const Ctrl* c) {
    std::vector<Entry> es = Collect(c);
    int nCol = 0, nVar = 0;
    for (auto& e : es) {
        if (e.isColor) {
            ImGui::PushStyleColor((ImGuiCol)e.id, ImVec4(e.v0, e.v1, e.v2, e.v3));
            nCol++;
        } else {
            ImGui::PushStyleVar((ImGuiStyleVar)e.id, e.v0);
            nVar++;
        }
    }
    return nCol * 1000 + nVar;   // iki sayıyı paketle
}
void Pop(int packed) {
    int nVar = packed % 1000;
    int nCol = packed / 1000;
    if (nVar > 0) ImGui::PopStyleVar(nVar);
    if (nCol > 0) ImGui::PopStyleColor(nCol);
}

// ---------------------------------------------------------------------------
//  codegen yazımı
static std::string ColEnumName(int id) {
    switch ((ImGuiCol)id) {
    case ImGuiCol_Text: return "ImGuiCol_Text";
    case ImGuiCol_Button: return "ImGuiCol_Button";
    case ImGuiCol_ButtonHovered: return "ImGuiCol_ButtonHovered";
    case ImGuiCol_ButtonActive: return "ImGuiCol_ButtonActive";
    case ImGuiCol_FrameBg: return "ImGuiCol_FrameBg";
    case ImGuiCol_FrameBgHovered: return "ImGuiCol_FrameBgHovered";
    case ImGuiCol_FrameBgActive: return "ImGuiCol_FrameBgActive";
    case ImGuiCol_CheckMark: return "ImGuiCol_CheckMark";
    case ImGuiCol_SliderGrab: return "ImGuiCol_SliderGrab";
    case ImGuiCol_SliderGrabActive: return "ImGuiCol_SliderGrabActive";
    case ImGuiCol_PlotHistogram: return "ImGuiCol_PlotHistogram";
    case ImGuiCol_PlotLines: return "ImGuiCol_PlotLines";
    case ImGuiCol_ChildBg: return "ImGuiCol_ChildBg";
    case ImGuiCol_Border: return "ImGuiCol_Border";
    case ImGuiCol_TabSelected: return "ImGuiCol_TabSelected";
    case ImGuiCol_TableHeaderBg: return "ImGuiCol_TableHeaderBg";
    case ImGuiCol_Separator: return "ImGuiCol_Separator";
    case ImGuiCol_TextDisabled: return "ImGuiCol_TextDisabled";
    default: return "ImGuiCol_Text";
    }
}
static std::string VarEnumName(int id) {
    switch ((ImGuiStyleVar)id) {
    case ImGuiStyleVar_FrameRounding: return "ImGuiStyleVar_FrameRounding";
    case ImGuiStyleVar_FrameBorderSize: return "ImGuiStyleVar_FrameBorderSize";
    case ImGuiStyleVar_ChildRounding: return "ImGuiStyleVar_ChildRounding";
    case ImGuiStyleVar_Alpha: return "ImGuiStyleVar_Alpha";
    default: return "ImGuiStyleVar_Alpha";
    }
}
static std::string FNum4(float f) {
    char b[32];
    snprintf(b, sizeof b, "%.4g", (double)f);
    std::string r = b;
    if (r.find('.') == std::string::npos && r.find('e') == std::string::npos &&
        r.find("inf") == std::string::npos && r.find("nan") == std::string::npos)
        r += ".0";
    return r;
}
std::string EmitPush(const Ctrl* c) {
    std::vector<Entry> es = Collect(c);
    if (es.empty()) return "";
    std::ostringstream o;
    for (auto& e : es) {
        if (e.isColor)
            o << "    ImGui::PushStyleColor(" << ColEnumName(e.id) << ", ImVec4("
              << FNum4(e.v0) << "f, " << FNum4(e.v1) << "f, " << FNum4(e.v2) << "f, " << FNum4(e.v3) << "f));\n";
        else
            o << "    ImGui::PushStyleVar(" << VarEnumName(e.id) << ", " << FNum4(e.v0) << "f);\n";
    }
    return o.str();
}
std::string EmitPop(int packed) {
    int nVar = packed % 1000;
    int nCol = packed / 1000;
    if (nCol == 0 && nVar == 0) return "";
    std::ostringstream o;
    if (nVar > 0) o << "    ImGui::PopStyleVar(" << nVar << ");\n";
    if (nCol > 0) o << "    ImGui::PopStyleColor(" << nCol << ");\n";
    return o.str();
}

} // namespace objstyle
