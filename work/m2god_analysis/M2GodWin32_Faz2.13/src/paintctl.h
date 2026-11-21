// paintctl.h — FAZ 2.12: TEMA KONTROL ÇİZİM KATMANI (tasarımcı tarafı)
//
// Temalar AKTİVASYON BAĞLAMI KULLANMAZ —Button/CheckBox/RadioButton/GroupBox
// + (mac/modern) TextBox/ListBox TAMAMEN paintctl ile özel boyanır:
//
//   Apple & Mac : KAPSÜL Aqua buton (tam yuvarlak uçlar + parlak üst bant +
//                 odak halkası; default = mac mavisi), mac checkbox/radio
//                 (düz #0A7AFF + beyaz işaret), mac groupbox (NSBox), mac
//                 TextField/List kenarı (odakta mavi çerçeve) + yüzeyde trafik
//                 ışıklı başlık + #ECECEC form zemini.
//   Modern      : .NET 4.8 WinForms görünümü — beyaz→gri buton (hover mavi),
//                 WinForms checkbox/radio/groupbox, düz TextField kenarı +
//                 #F0F0F0 form zemini.
//   Oldschool   : özel çizim YOK (klasik Win32) — deri YERİNDEN sökülür.
//
// FAZ 2.12: temalar arasında geçiş pencere İMHA ETMEDEN yapılır:
// Apply = deriyi YERİNDE değiştir (skin güncelle / tak / sök). Çökme nedeni
// olan "tüm panelleri yok et + bağlam değiştir + yeniden yarat" döngüsü ve
// her denetim yaratımında Push/Pop bağlam geçişi KALDIRILDI.
//
// Güvenlik: alt sınıflama KOMUTLA yapılır (SetWindowLongPtrW/GWLP_WNDPROC —
// eski proc pencere ÖZELLİĞİNDE saklanır ve DERİ SÖKÜLSE DE DURUR: passthrough
// zincir bozulmaz). comctl32 SetWindowSubclass kullanılmaz.
// Üretilen uygulamadaki karşılığı global_template.inc içindeki CtlPaint'tir
// (aynı paletler, iki kopya — tasarımcı ve üretilen exe birbirinden bağımsız).
#pragma once
#include "app.h"

namespace paintctl {

// temanın varsayılan form zemini (BackColor="Control" çözümü):
// Modern → #F0F0F0, Mac → #ECECEC, diğer → beyaz (eski davranış)
COLORREF ThemeBack(int theme);

// bir kontrolün EBEVEYNİNİN etkin zemin rengi (modelden; GroupBox/Panel
// BackColor'ı zincirlenir, yüzeye kadar iner; bulunamazsa tema zemini)
COLORREF ParentBack(HWND h, int theme);

// kontrolü temaya göre özel boyamaya geçirir (Button/CheckBox/RadioButton/
// GroupBox + Modern/Mac'ta TextBox/ListBox ailesi; diğer tipler false döner).
// skin ZATEN takılıysa: tema Modern/Mac → flavor YERİNDEN güncellenir;
// tema Oldschool → deri YERİNDEN SÖKÜLÜR (procedür zinciri bozulmadan
// passthrough'a döner). Hiçbir pencere imha edilmez — çökme güvenliği.
bool Apply(HWND h, const wchar_t* type, int theme);

} // namespace paintctl
