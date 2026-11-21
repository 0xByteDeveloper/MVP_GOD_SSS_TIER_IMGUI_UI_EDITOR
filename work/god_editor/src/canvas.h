// ============================================================================
//  canvas.h — tasarım yüzeyi: WYSIWYG ImGui formu + seçim + taşıma + boyutlandırma
//  Canvas, üretilen kodla BİREBİR aynı ImGui çağrı dizisini kullanır.
// ============================================================================
#pragma once
#include "imgod.h"
#include "model.h"
#include "imgui.h"

namespace canvas {

// merkez paneli çiz (sınırlayıcı dikdörtgen canvas alanı)
void Draw(const ImVec2& rectMin, const ImVec2& rectMax);

// araç kutusundan arm edilmiş yerleştirme modu
extern std::string g_armTool;      // boş değilse sonraki tık o nesneyi yerleştirir
extern Ctrl* g_armParent;          // yerleştirme konteyneri (null = form seviyesi)

// seçim
void Select(Ctrl* c);
void SelectForm();
void ClickSelectUnder(ImVec2 pt);  // pt: canvas koordinatı — alttaki nesneyi seçer

// seçili nesnenin canvas koordinatındaki mutlak dikdörtgeni (form-göreli x/y + ebeveyn zinciri)
ImVec2 CtrlAbsPos(const Ctrl* c);

} // namespace canvas
