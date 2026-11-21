// codetree.h — FAZ 2.8: KOD GEZGİNİ (sol üst panel)
// RAD Studio "Structure View" tarzı: projedeki formlar → kontroller →
// AÇILMIŞ olay işleyicileri (button1 → button1_Click gibi) ağaç görünümü.
// Tek tık: forma geç + kontrolü seç (+ olaylar sekmesi);
// Çift tık: KODA GİT — VS2019 bağlıysa gerçek koda, değilse üretilen kodu gösterir.
#pragma once
#include "app.h"

namespace codetree {

// ağaçtan ana pencereye taşınan gezinme bilgisi (heap'te post edilir, alan mainwnd)
struct NavInfo {
    int  formIdx = 0;          // hedef formun indeksi
    bool isForm = false;       // form düğümü mü
    wchar_t ctrlName[64] = L""; // boşsa form düzeyi olay
    bool isHandler = false;     // olay işleyici düğümü mü
    wchar_t evName[64] = L"";
    wchar_t handler[96] = L"";
};

HWND Create(HWND parent);

// ağacı yeniden doldur (seçim / model / form değişince mainwnd çağırır)
void Refresh();

} // namespace codetree
