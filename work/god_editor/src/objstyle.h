// ============================================================================
//  objstyle.h — nesne başına stil/renk geçersiz kılmaları.
//  Canvas VE kod üreticisi AYNI koleksiyonu kullanır → gerçek WYSIWYG:
//  editörde PushStyleColor/PushStyleVar dizisi neyse, üretilen .cpp'de de o.
// ============================================================================
#pragma once
#include "model.h"

namespace objstyle {

// tek bir stil girişi (renk veya stil değişkeni)
struct Entry {
    bool isColor = false;
    int  id = 0;                 // ImGuiCol_* veya ImGuiStyleVar_*
    float v0 = 0, v1 = 0, v2 = 0, v3 = 0;   // ImVec4 değeri
    std::string idName;          // codegen için "ImGuiCol_Button" / "ImGuiStyleVar_FrameRounding"
    std::string propName;        // kaynak prop ("ButtonColor")
};

// Ctrl props'tan uygulanacak stil girişlerini topla (sıralı).
std::vector<Entry> Collect(const Ctrl* c);

// kod üretimi için: kaç renk/stil push'u uygulanır (packed — Push ile aynı)
int  CountPushes(const Ctrl* c);

// canvas uygulaması: kaç giriş itildi ise döner
int  Push(const Ctrl* c);
void Pop(int n);

// codegen yazımı: aynı girişlerin C++ kaynak satırları
std::string EmitPush(const Ctrl* c);       // "ImGui::PushStyleColor(...);\n..." (4 boşluk girinti)
std::string EmitPop(int n);                // "ImGui::PopStyleColor(n);\n..."

// yardımcılar
bool  ParseHexColor(const std::string& s, float out[4]);   // "#RRGGBB" → 0..1
std::string HexOf(const float rgba[4]);                    // 0..1 → "#RRGGBB"

} // namespace objstyle
