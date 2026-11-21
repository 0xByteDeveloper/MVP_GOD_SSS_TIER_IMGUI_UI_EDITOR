// theme.h — FAZ 2.13: TEMA SİSTEMİ (7 tema — M2God GRAFİT 4'LÜSÜ EKLENDİ)
//   0 = Oldschool  : klasik Win32 — comctl32 v5 görünümü, hiçbir özel çizim yok
//   1 = Modern     : .NET 4.8 / C++ CLR WinForm benzeri — paintctl özel çizim
//   2 = Apple & Mac: GERÇEK AQUA görünümü — kapsül buton, trafik ışıkları
//   3 = Ruby Graphite   : M2God teması (Faz2.5 programından birebir) —
//   4 = Yellow Graphite :   5 duraklı gradyan buton (normal/hover/basılmış),
//   5 = Purple Graphite :   #252525 zemin, koyu alan/düğme kenarları,
//   6 = Dark            :   başlık çubuğu tema gradyanı (Ruby kırmızı v.b.)
//   BUTON FALAN HER ŞEYİ KAPSAR: Button / CheckBox / RadioButton / GroupBox /
//   TextBox / ListBox + form zemini + başlık çubuğu (bkz. paintctl + surface).
//
// Tema = saf paintctl çizimidir; aktivasyon bağlamı / pencere imha YOK →
// kaç kez değiştirilirse değiştirilsin ÇÖKMEZ (Faz 2.12 mimarisi korundu).
#pragma once
#include "app.h"

namespace theme {

// tema kimlikleri (dosyada/kodda "Oldschool"/"Modern"/"AppleMac" ve
// M2God adları "Ruby"/"Yellow"/"Purple"/"Dark" yazılır — uzun adlar da kabul)
enum ThemeId {
    OLDSCHOOL = 0, MODERN = 1, APPLEMAC = 2,
    RUBY = 3, YELLOW = 4, PURPLE = 5, DARKM = 6
};

// ilk M2God grafit teması kimliği (Ruby..Dark aralığı)
const int FIRST_GRAPHITE = 3;
const int LAST_THEME = 6;

// M2God grafit teması mı? (Ruby/Yellow/Purple/Dark)
bool IsGraphite(int t);

// şu an editörde etkin tema
int Current();

// projenin varsayılan teması (proje dosyasına kaydedilir, kod üretiminde kullanılır)
int Project();
void SetProject(int t);

// temayı değiştir: editör durumunu günceller (mainWnd: menü yeniden çizimi;
// 0 olabilir). Gerçek denetim derilemesi mainwnd DoThemeSwitch + paintctl
// ile YERİNDE yapılır — pencere yaratımı/imha yok (çökme düzeltmesi).
void Switch(int t, HWND mainWnd);

// nesnenin "Theme" özelliği metninden ETKİN tema:
// "Modern"→1, "AppleMac"/"Apple & Mac"→2, "Ruby"/"Ruby Graphite"→3,
// "Yellow"/"Yellow Graphite"→4, "Purple"→5, "Dark"→6, "Oldschool"→0,
// BOŞ / "(Proje Varsayılanı)" / bilinmeyen → -1 (kalıtsal → editör teması)
int Effective(const wchar_t* themeVal);

// ad ↔ kimlik dönüşümü ("AppleMac" ve "Apple & Mac" yazımı ikisi de kabul;
// "RadStudio"/"RAD Studio" bilinen ad DEĞİLDİR — eski proje dosyaları
// Oldschool'a düşer). M2God uzun adları ("Ruby Graphite" v.b.) da kabul.
int  IdOfName(const wchar_t* name);
const wchar_t* NameOf(int t);        // "Oldschool"/"Modern"/"Apple & Mac"/"Ruby Graphite"...
const wchar_t* SaveNameOf(int t);    // .m2god'da saklanan kısa ad ("AppleMac","Ruby"...)
const wchar_t* MenuNameOf(int t);    // Tema menüsünde gösterilen uzun açıklamalı ad

// temanın tasarım yüzeyi varsayılan form zemin rengi (BackColor="Control" iken)
// Modern → #F0F0F0, AppleMac → #ECECEC, M2God grafit → #252525 (CLR_FORM_BACK),
// Oldschool → beyaz (eski davranış)
COLORREF FormBackColorOf(int t);

// M2God grafit temaların form/başlık çubuğu renkleri (Faz2.5 theme.h birebir):
// başlık çubuğu gradyanı (üst/alt durak) — Ruby: (118,3,0)/(53,1,0) v.b.
COLORREF TitleGradTop(int t);
COLORREF TitleGradBottom(int t);

// comctl32 v6 manifest XML'i — yalnızca ÜRETİLEN kodun Global.h'ında bilgi
// amaçlı yer alır (tasarımcı artık çalışma anında bağlam kullanmaz)
const char* ManifestXmlA();

} // namespace theme
