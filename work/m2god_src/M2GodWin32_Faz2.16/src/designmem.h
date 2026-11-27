// designmem.h — FAZ 2.16: TASARIM HAFIZASI KATMANI
//
// Kullanıcı raporu (Faz 2.15): "VS'ye bağlanınca 'Bu Proje İçin Kayıtlı
// Tasarım Var' diyaloğu HER SEFERİNDE çıkıyor. Form1.m2god, Form2.m2god
// görüyorum — 2.12/2.13'te yeni tema çalışmadığı için sürekli 'yeni tasarım
// oluştur'a basmıştım, o çöp tasarımlar sürekli önüme geliyor."
//
// Bu katman o bug'ı kökten çözen üç parçayı taşır:
//   1) PROJE HAFIZASI — bağlı proje klasörüne ".m2god_last" işaret dosyası
//      yazılır: o projede SON kullanılan tasarımın adı. Yeniden bağlanınca
//      diyalog ÇIKMAZ, tasarım sessizce otomatik yüklenir.
//   2) TASARIM İSTATİSTİĞİ — her .m2god hafifçe taranır: kaç form, kaç obje,
//      hangi tema. "BOŞ tasarım" (0 obje) tespiti sayesinde 2.12/2.13
//      döneminden kalan çöp tasarımlar tek tuşla temizlenir.
//   3) OTURUM HAFIZASI — %APPDATA%\M2GodWin32\last_session.txt: editörü
//      yeniden açınca "son tasarım" tek tuşla geri gelir (eski menü
//      karışıklığı da biter).
//
// Katman bilinçli olarak mainwnd.cpp monolitinden AYRI birimdedir:
// tests/ptr_test16.cpp bu dosyaları doğrudan link edip test eder.
#pragma once
#include <string>

namespace designmem {

// Bir .m2god dosyasının hafif istatistiği (tam yükleme YOK — sadece JSON tarama)
struct DesignStats {
    int         forms = 0;      // projedeki form sayısı (eski tek-formlu dosyada 1)
    int         objects = 0;   // TÜM formlardaki toplam obje sayısı
    std::wstring theme;        // "Modern" / "AppleMac" ... (yoksa boş)
    bool        ok = false;    // dosya açılıp çözümlenebildi mi

    bool IsEmpty() const { return ok && objects == 0; }   // "çöp tasarım"
};

// .m2god dosyasını tara (format m2god-project v3 VEYA eski m2god-form v2 —
// geriye dönük uyum: hangi fazda yazılmış olursa olsun okunur)
DesignStats Describe(const std::wstring& m2godPath);

// Seçim diyalogundaki liste satırı:
//   "Form2.m2god — 3 form, 21 obje, Modern"
//   "Form1.m2god — BOŞ tasarım (0 obje) — çöp"
//   "Form3.m2god — (okunamadı)"
std::wstring DescribeLine(const std::wstring& fileName, const DesignStats& st);

// --- 1) PROJE HAFIZASI (bağlı proje klasöründeki .m2god_last) ----------------
// projDir altına "son kullanılan tasarımın DOSYA ADI"nı yazar.
// Başarısızsa false (klasör salt-okunur olabilir → o zaman diyalog akışı
// eski haliyle çalışmaya devam eder, hiçbir şey bozulmaz).
bool RememberForProject(const std::wstring& projDir, const std::wstring& designFile);

// Son kullanılan tasarımın adı (yoksa / dosya artık yoksa boş döner).
// Dönen değer HER ZAMAN salt dosya adıdır (içinde \ veya / barındıramaz —
// işaret dosyası elle kurcalansa bile yol enjeksiyonu engellenir).
std::wstring LastForProject(const std::wstring& projDir);

// --- 2) OTURUM HAFIZASI (%APPDATA%\M2GodWin32\last_session.txt) -------------
bool RememberSession(const std::wstring& designFile);   // tam yol yazılır
std::wstring LastSession();                             // kayıtlı tam yol (yoksa L"")

} // namespace designmem
