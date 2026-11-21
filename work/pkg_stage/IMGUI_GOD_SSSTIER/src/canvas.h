// ============================================================================
//  canvas.h — tasarım yüzeyi: WYSIWYG ImGui formu + çoklu seçim + taşıma +
//  boyutlandırma + rubber band + imleç geri bildirimi.
//  Canvas, üretilen kodla BİREBİR aynı ImGui çağrı dizisini kullanır:
//    * form ÜRETİM temasıyla çizilir (theme::PushProduction)
//    * nesne başına stil/renk Push'ları objstyle ile (kod üretimiyle aynı)
//    * tüm widget kimlikleri "metin##ad" — ImGui ID çakışması imkansız
// ============================================================================
#pragma once
#include "imgod.h"
#include "model.h"
#include "imgui.h"

namespace canvas {

// merkez paneli çiz (sınırlayıcı dikdörtgen canvas alanı)
void Draw(const ImVec2& rectMin, const ImVec2& rectMax);

// araç kutusundan arm edilmiş yerleştirme modu (ESC ile iptal)
extern std::string g_armTool;      // boş değilse form tıklaması o nesneyi yerleştirir
extern Ctrl* g_armParent;          // yerleştirme konteyneri (null = form seviyesi)

// arm yardımcıları (araç kutusu tık / çift tık)
void ArmTool(const std::string& tool);
bool IsArmed();
void CancelArm();
// çift tık: nesneyi aktif forma anında ekle (kademeli konum)
bool AddArmedToForm();

// seçim (çoklu seçim modelde — buradan senkronlanır)
void Select(Ctrl* c, bool additive = false);
void SelectForm();
void ClickSelectUnder(ImVec2 pt, bool additive);   // pt: ekran koordinatı — üstteki nesneyi seçer

// seçili nesnenin form-göreli konumu (ebeveyn zinciri toplanır)
ImVec2 CtrlAbsPos(const Ctrl* c);

// selftest/otomasyon kancaları: gerçek çizim dikdörtgenleri (ekran koordinatı)
void GetFormRect(float* x, float* y, float* w, float* h);          // tasarım formunun gerçek dikdörtgeni
bool ObjRectOf(const std::string& name, float* x, float* y, float* w, float* h);   // nesnenin gerçek dikdörtgeni

} // namespace canvas
