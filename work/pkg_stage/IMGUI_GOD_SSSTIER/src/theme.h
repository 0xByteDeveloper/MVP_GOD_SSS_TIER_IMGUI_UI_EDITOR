// ============================================================================
//  theme.h — ImGui tema sistemi.
//  Kullanıcının açık talebi: Ruby/Yellow/Purple Graphite gibi Win32API'ye özel
//  temalar YOK — bunlar Win32API çizim dünyasına aitti. ImGui evreninde
//  ImGui'nin kendi tema seti + 3 GOD SSS TIER özel paleti geçerli.
// ============================================================================
#pragma once
#include <string>
#include <vector>

namespace theme {

enum ThemeId {
    TImGuiDark = 0,     // ImGui Dark (varsayılan)
    TImGuiLight = 1,    // ImGui Light
    TImGuiClassic = 2,  // ImGui Classic (1.2 tarzı)
    TGodOps = 3,        // GOD OPS — askeri komuta konsolu (yeşil fosfor)
    TMidnightSteel = 4, // Midnight Steel — koyu çelik mavi
    TAmmoTan = 5,       // Ammo Tan — askeri haki/kum tonu
    TCount = 6
};

struct ThemeInfo {
    int id;
    const char* name;
    const char* desc;
};
const std::vector<ThemeInfo>& List();
const char* NameOf(int id);
void Apply(int id);                     // ImGui stiline uygular (ImGui::CreateContext sonrası)

// üretilen koda taşınan tema çağrısı ("ImGui::StyleColorsDark();" veya "IGOD::ApplyGodOpsTheme();")
std::string GenThemeApiCall(int id);

// WYSIWYG: tasarım yüzeyi, ÜRETİM temasıyla çizilir (editör temasıyla değil).
// PushProduction: mevcut ImGuiStyle'ı yedekler + üretim temasını uygular.
// PopProduction: yedeği geri yükler. (Yuvalanabilir.)
void PushProduction(int id);
void PopProduction();

} // namespace theme
