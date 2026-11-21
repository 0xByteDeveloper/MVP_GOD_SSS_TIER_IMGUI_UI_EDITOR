// forms.h — FAZ 2.7: çok formlu proje katmanı
// Tek projede birden çok form (Form1, Form2, ...). g_model her zaman AKTİF formu
// temsil eder; form değiştirilirken aktif form belgeye "park edilir"
// (JSON + kendi Geri Al/Yinele yığınları), hedef form belgeden geri yüklenir.
// RAD Studio / VS2019 .NET çok formlu proje modeline karşılık gelir.
#pragma once
#include <string>
#include <vector>

namespace forms {

// Taze proje: tek boş Form1 (başlangıç / Yeni Tasarım / yeni bağlantı)
void InitNew();

int   Count();                       // projedeki form sayısı (>= 1)
int   ActiveIndex();                 // aktif formun indeksi
std::wstring ActiveName();           // aktif formun adı ("Form1"...)
std::vector<std::wstring> Names();   // listede gösterilecek adlar (sıralı)
std::wstring NextFreeName();         // "FormN" — çakışmayan ilk boş ad

// Yeni form ekler ve ona geçer. Başarılıysa yeni formun adı döner.
std::wstring AddNew();

// Aktif formu değiştirir (park/unpark). Değişmediyse/hatalıysa false.
// NOT: HWND'leri çağıran taraf surface::RecreateAll ile yeniden kurmalı.
bool SwitchTo(int index);

// Aktif formu yeniden adlandırır (geçersiz ad → false)
bool RenameActive(const std::wstring& newName);

// Formu siler (son form silinemez → false). Aktif form siliniyorsa komşuya geçer.
bool RemoveAt(int index);

// Ad geçerli mi: kimlik karakteri + boş değil + projede benzersiz
bool ValidName(const std::wstring& n);

// Kaydet/yükle — tüm formlar TEK .m2god içinde. Eski (tek formlu) dosyalar
// otomatik olarak tek formlu projeye dönüşür (geriye dönük uyum).
bool SaveProject(const std::wstring& path);
bool LoadProject(const std::wstring& path);

// FAZ 2.13 — KLASÖR PROJESİ: Forms.info (kaç form + adlar + tema) ve her
// forma bir FormN.info dosyası (aynı tasarım JSON'u). Faz2.5 M2God projesi
// bu düzenle UI editörde açılır; Dosya → Klasör Projesi Aç/Kaydet kullanır.
bool SaveFolderProject(const std::wstring& dir);
bool LoadFolderProject(const std::wstring& dir);

// FAZ 2.10 — TEMA: projenin varsayılan teması
// (0=Oldschool, 1=Modern, 2=Apple & Mac, 3=Ruby, 4=Yellow, 5=Purple, 6=Dark).
// .m2god / Forms.info dosyasına kaydedilir; üretilen koda başlangıç teması
// olarak taşınır.
int  ThemeId();
void SetThemeId(int t);

// FAZ 2.9 — KOD GEZGİNİ: herhangi bir formun TÜM objeleri (aktif form g_model'den,
// parklı formlar tasarım JSON'undan okunur). parentIdx aynı vektörde üst objenin
// indeksi; konteyner olmayanlar / form düzeyi için -1.
struct TreeObj {
    std::wstring name;
    std::wstring type;
    bool nonVisual = false;
    std::vector<std::pair<std::wstring, std::wstring>> events;   // olay → handler
    int parentIdx = -1;
};
std::vector<TreeObj> TreeObjectsOf(int formIdx);
// form düzeyi olay → handler listesi
std::vector<std::pair<std::wstring, std::wstring>> TreeFormEventsOf(int formIdx);

} // namespace forms
