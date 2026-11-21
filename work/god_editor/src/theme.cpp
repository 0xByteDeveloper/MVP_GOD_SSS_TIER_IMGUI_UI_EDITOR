// theme.cpp — ImGui temaları (3 yerleşik + 3 GOD SSS TIER özel paleti)
#include "theme.h"
#include "imgui.h"

namespace theme {

static std::vector<ThemeInfo> g_list = {
    { TImGuiDark,     "ImGui Dark",      "ImGui yerleşik koyu tema" },
    { TImGuiLight,   "ImGui Light",     "ImGui yerleşik açık tema" },
    { TImGuiClassic,  "ImGui Classic",   "ImGui klasik (1.2) görünümü" },
    { TGodOps,       "GOD OPS",         "Askeri komuta konsolu — yeşil fosfor CRT havası" },
    { TMidnightSteel,"Midnight Steel",  "Koyu çelik mavi — gece operasyon paneli" },
    { TAmmoTan,      "Ammo Tan",        "Askeri haki/kum — saha terminali" },
};
const std::vector<ThemeInfo>& List() { return g_list; }
const char* NameOf(int id) {
    for (auto& t : g_list) if (t.id == id) return t.name;
    return "ImGui Dark";
}

// GOD OPS — fosfor yeşili komuta konsolu
static void ApplyGodOps() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 3.0f;
    s.FrameRounding = 2.0f;
    s.GrabRounding = 2.0f;
    s.ScrollbarRounding = 3.0f;
    s.TabRounding = 3.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                   = ImVec4(0.80f, 1.00f, 0.80f, 1.00f);
    c[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.60f, 0.40f, 1.00f);
    c[ImGuiCol_WindowBg]               = ImVec4(0.030f, 0.060f, 0.040f, 0.96f);
    c[ImGuiCol_ChildBg]                = ImVec4(0.040f, 0.080f, 0.050f, 1.00f);
    c[ImGuiCol_PopupBg]                = ImVec4(0.020f, 0.050f, 0.030f, 0.98f);
    c[ImGuiCol_Border]                 = ImVec4(0.25f, 0.55f, 0.30f, 0.60f);
    c[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.060f, 0.130f, 0.080f, 1.00f);
    c[ImGuiCol_FrameBgHovered]         = ImVec4(0.100f, 0.200f, 0.120f, 1.00f);
    c[ImGuiCol_FrameBgActive]          = ImVec4(0.140f, 0.280f, 0.160f, 1.00f);
    c[ImGuiCol_TitleBg]                = ImVec4(0.040f, 0.090f, 0.050f, 1.00f);
    c[ImGuiCol_TitleBgActive]          = ImVec4(0.070f, 0.150f, 0.090f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.030f, 0.060f, 0.040f, 0.60f);
    c[ImGuiCol_MenuBarBg]              = ImVec4(0.040f, 0.080f, 0.050f, 1.00f);
    c[ImGuiCol_ScrollbarBg]            = ImVec4(0.030f, 0.060f, 0.040f, 0.60f);
    c[ImGuiCol_ScrollbarGrab]          = ImVec4(0.180f, 0.380f, 0.220f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.240f, 0.480f, 0.280f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.300f, 0.600f, 0.340f, 1.00f);
    c[ImGuiCol_CheckMark]              = ImVec4(0.50f, 1.00f, 0.55f, 1.00f);
    c[ImGuiCol_SliderGrab]             = ImVec4(0.320f, 0.640f, 0.360f, 1.00f);
    c[ImGuiCol_SliderGrabActive]       = ImVec4(0.420f, 0.840f, 0.470f, 1.00f);
    c[ImGuiCol_Button]                 = ImVec4(0.090f, 0.190f, 0.110f, 1.00f);
    c[ImGuiCol_ButtonHovered]           = ImVec4(0.140f, 0.280f, 0.160f, 1.00f);
    c[ImGuiCol_ButtonActive]           = ImVec4(0.200f, 0.400f, 0.230f, 1.00f);
    c[ImGuiCol_Header]                 = ImVec4(0.080f, 0.170f, 0.100f, 1.00f);
    c[ImGuiCol_HeaderHovered]           = ImVec4(0.130f, 0.260f, 0.150f, 1.00f);
    c[ImGuiCol_HeaderActive]           = ImVec4(0.180f, 0.360f, 0.210f, 1.00f);
    c[ImGuiCol_Separator]              = ImVec4(0.25f, 0.55f, 0.30f, 0.50f);
    c[ImGuiCol_SeparatorHovered]       = ImVec4(0.350f, 0.700f, 0.400f, 0.78f);
    c[ImGuiCol_SeparatorActive]         = ImVec4(0.450f, 0.900f, 0.500f, 1.00f);
    c[ImGuiCol_ResizeGrip]              = ImVec4(0.25f, 0.55f, 0.30f, 0.25f);
    c[ImGuiCol_ResizeGripHovered]       = ImVec4(0.350f, 0.700f, 0.400f, 0.67f);
    c[ImGuiCol_ResizeGripActive]        = ImVec4(0.450f, 0.900f, 0.500f, 0.95f);
    c[ImGuiCol_Tab]                     = ImVec4(0.070f, 0.150f, 0.090f, 1.00f);
    c[ImGuiCol_TabHovered]              = ImVec4(0.150f, 0.300f, 0.170f, 1.00f);
    c[ImGuiCol_TabSelected]            = ImVec4(0.110f, 0.230f, 0.130f, 1.00f);
    c[ImGuiCol_TabDimmed]              = ImVec4(0.050f, 0.100f, 0.060f, 1.00f);
    c[ImGuiCol_TabDimmedSelected]      = ImVec4(0.080f, 0.170f, 0.100f, 1.00f);
    c[ImGuiCol_PlotLines]               = ImVec4(0.50f, 1.00f, 0.55f, 1.00f);
    c[ImGuiCol_PlotLinesHovered]        = ImVec4(0.70f, 1.00f, 0.75f, 1.00f);
    c[ImGuiCol_PlotHistogram]           = ImVec4(0.320f, 0.640f, 0.360f, 1.00f);
    c[ImGuiCol_PlotHistogramHovered]    = ImVec4(0.420f, 0.840f, 0.470f, 1.00f);
    c[ImGuiCol_TableHeaderBg]           = ImVec4(0.060f, 0.130f, 0.080f, 1.00f);
    c[ImGuiCol_TableBorderStrong]       = ImVec4(0.25f, 0.55f, 0.30f, 0.50f);
    c[ImGuiCol_TableBorderLight]        = ImVec4(0.20f, 0.45f, 0.25f, 0.30f);
    c[ImGuiCol_TextSelectedBg]          = ImVec4(0.180f, 0.400f, 0.220f, 0.45f);
    c[ImGuiCol_DragDropTarget]          = ImVec4(0.450f, 0.900f, 0.500f, 0.90f);
}
// Midnight Steel — koyu çelik mavi
static void ApplyMidnightSteel() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.TabRounding = 4.0f;
    s.WindowBorderSize = 1.0f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                   = ImVec4(0.86f, 0.92f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled]           = ImVec4(0.48f, 0.55f, 0.66f, 1.00f);
    c[ImGuiCol_WindowBg]               = ImVec4(0.070f, 0.090f, 0.140f, 0.97f);
    c[ImGuiCol_ChildBg]                = ImVec4(0.090f, 0.110f, 0.170f, 1.00f);
    c[ImGuiCol_PopupBg]                = ImVec4(0.050f, 0.070f, 0.120f, 0.98f);
    c[ImGuiCol_Border]                 = ImVec4(0.30f, 0.38f, 0.55f, 0.60f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.120f, 0.150f, 0.230f, 1.00f);
    c[ImGuiCol_FrameBgHovered]         = ImVec4(0.180f, 0.220f, 0.340f, 1.00f);
    c[ImGuiCol_FrameBgActive]          = ImVec4(0.230f, 0.280f, 0.430f, 1.00f);
    c[ImGuiCol_TitleBg]                = ImVec4(0.080f, 0.100f, 0.160f, 1.00f);
    c[ImGuiCol_TitleBgActive]          = ImVec4(0.130f, 0.160f, 0.250f, 1.00f);
    c[ImGuiCol_MenuBarBg]              = ImVec4(0.090f, 0.110f, 0.170f, 1.00f);
    c[ImGuiCol_CheckMark]              = ImVec4(0.55f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]             = ImVec4(0.45f, 0.60f, 0.95f, 1.00f);
    c[ImGuiCol_SliderGrabActive]       = ImVec4(0.60f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_Button]                 = ImVec4(0.150f, 0.185f, 0.290f, 1.00f);
    c[ImGuiCol_ButtonHovered]           = ImVec4(0.220f, 0.270f, 0.420f, 1.00f);
    c[ImGuiCol_ButtonActive]           = ImVec4(0.300f, 0.370f, 0.560f, 1.00f);
    c[ImGuiCol_Header]                 = ImVec4(0.140f, 0.170f, 0.260f, 1.00f);
    c[ImGuiCol_HeaderHovered]           = ImVec4(0.200f, 0.240f, 0.380f, 1.00f);
    c[ImGuiCol_HeaderActive]           = ImVec4(0.260f, 0.310f, 0.480f, 1.00f);
    c[ImGuiCol_Tab]                     = ImVec4(0.120f, 0.150f, 0.240f, 1.00f);
    c[ImGuiCol_TabHovered]              = ImVec4(0.230f, 0.280f, 0.440f, 1.00f);
    c[ImGuiCol_TabSelected]             = ImVec4(0.180f, 0.220f, 0.350f, 1.00f);
    c[ImGuiCol_TabDimmed]              = ImVec4(0.100f, 0.120f, 0.190f, 1.00f);
    c[ImGuiCol_TabDimmedSelected]      = ImVec4(0.140f, 0.170f, 0.270f, 1.00f);
    c[ImGuiCol_PlotLines]               = ImVec4(0.55f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_PlotHistogram]           = ImVec4(0.45f, 0.60f, 0.95f, 1.00f);
    c[ImGuiCol_TableHeaderBg]           = ImVec4(0.110f, 0.140f, 0.220f, 1.00f);
    c[ImGuiCol_TextSelectedBg]          = ImVec4(0.260f, 0.320f, 0.500f, 0.45f);
}
// Ammo Tan — askeri haki/kum
static void ApplyAmmoTan() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 2.0f;
    s.FrameRounding = 2.0f;
    s.GrabRounding = 2.0f;
    s.TabRounding = 2.0f;
    s.FrameBorderSize = 1.0f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                   = ImVec4(0.88f, 0.84f, 0.74f, 1.00f);
    c[ImGuiCol_TextDisabled]           = ImVec4(0.55f, 0.51f, 0.42f, 1.00f);
    c[ImGuiCol_WindowBg]               = ImVec4(0.130f, 0.120f, 0.090f, 0.97f);
    c[ImGuiCol_ChildBg]                = ImVec4(0.160f, 0.150f, 0.110f, 1.00f);
    c[ImGuiCol_PopupBg]                = ImVec4(0.110f, 0.100f, 0.080f, 0.98f);
    c[ImGuiCol_Border]                 = ImVec4(0.45f, 0.42f, 0.30f, 0.70f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.210f, 0.195f, 0.150f, 1.00f);
    c[ImGuiCol_FrameBgHovered]         = ImVec4(0.280f, 0.260f, 0.200f, 1.00f);
    c[ImGuiCol_FrameBgActive]          = ImVec4(0.350f, 0.325f, 0.250f, 1.00f);
    c[ImGuiCol_TitleBg]                = ImVec4(0.170f, 0.155f, 0.120f, 1.00f);
    c[ImGuiCol_TitleBgActive]          = ImVec4(0.240f, 0.220f, 0.170f, 1.00f);
    c[ImGuiCol_MenuBarBg]              = ImVec4(0.170f, 0.155f, 0.120f, 1.00f);
    c[ImGuiCol_CheckMark]              = ImVec4(0.95f, 0.80f, 0.40f, 1.00f);
    c[ImGuiCol_SliderGrab]             = ImVec4(0.750f, 0.640f, 0.360f, 1.00f);
    c[ImGuiCol_SliderGrabActive]       = ImVec4(0.950f, 0.800f, 0.450f, 1.00f);
    c[ImGuiCol_Button]                 = ImVec4(0.230f, 0.215f, 0.165f, 1.00f);
    c[ImGuiCol_ButtonHovered]           = ImVec4(0.320f, 0.295f, 0.220f, 1.00f);
    c[ImGuiCol_ButtonActive]           = ImVec4(0.430f, 0.395f, 0.290f, 1.00f);
    c[ImGuiCol_Header]                 = ImVec4(0.220f, 0.205f, 0.155f, 1.00f);
    c[ImGuiCol_HeaderHovered]           = ImVec4(0.310f, 0.285f, 0.210f, 1.00f);
    c[ImGuiCol_HeaderActive]           = ImVec4(0.400f, 0.365f, 0.270f, 1.00f);
    c[ImGuiCol_Tab]                     = ImVec4(0.200f, 0.185f, 0.140f, 1.00f);
    c[ImGuiCol_TabHovered]              = ImVec4(0.330f, 0.305f, 0.230f, 1.00f);
    c[ImGuiCol_TabSelected]             = ImVec4(0.270f, 0.250f, 0.185f, 1.00f);
    c[ImGuiCol_TabDimmed]              = ImVec4(0.170f, 0.155f, 0.120f, 1.00f);
    c[ImGuiCol_TabDimmedSelected]      = ImVec4(0.220f, 0.205f, 0.155f, 1.00f);
    c[ImGuiCol_PlotLines]               = ImVec4(0.95f, 0.80f, 0.40f, 1.00f);
    c[ImGuiCol_PlotHistogram]           = ImVec4(0.750f, 0.640f, 0.360f, 1.00f);
    c[ImGuiCol_TableHeaderBg]           = ImVec4(0.190f, 0.175f, 0.135f, 1.00f);
    c[ImGuiCol_TextSelectedBg]          = ImVec4(0.400f, 0.365f, 0.270f, 0.45f);
}

void Apply(int id) {
    switch (id) {
    case TImGuiLight: ImGui::StyleColorsLight(); break;
    case TImGuiClassic: ImGui::StyleColorsClassic(); break;
    case TGodOps: ImGui::StyleColorsDark(); ApplyGodOps(); break;
    case TMidnightSteel: ImGui::StyleColorsDark(); ApplyMidnightSteel(); break;
    case TAmmoTan: ImGui::StyleColorsDark(); ApplyAmmoTan(); break;
    case TImGuiDark:
    default: ImGui::StyleColorsDark(); break;
    }
}

std::string GenThemeApiCall(int id) {
    switch (id) {
    case TImGuiLight: return "ImGui::StyleColorsLight();";
    case TImGuiClassic: return "ImGui::StyleColorsClassic();";
    case TGodOps: return "IGOD::ApplyGodOpsTheme();";
    case TMidnightSteel: return "IGOD::ApplyMidnightSteelTheme();";
    case TAmmoTan: return "IGOD::ApplyAmmoTanTheme();";
    default: return "ImGui::StyleColorsDark();";
    }
}

} // namespace theme
