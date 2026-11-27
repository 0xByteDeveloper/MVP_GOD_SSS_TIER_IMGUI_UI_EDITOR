# M2GodWin32 — Faz 2.16 (KAYITLI TASARIM HAFIZASI)

Faz 2.13 kaynak ağacının üzerine, kullanıcının bildirdiği **"bağlanınca
kayıtlı tasarım diyaloğu her seferinde çıkıyor"** bug'ının tam çözümü.

## Kullanıcı Raporu (Faz 2.15'te)

> "M2God Vs ye bağla dedim, 'Bu proje için kayıtlı tasarım var' dedi.
> Form1.m2god, Form2.m2god olarak görüyorum. Faz2.15'den önceki eski bir
> sürümde — 2.13 veya 2.12'de — yeni tema çalışmıyordu diye yeni tasarım
> oluştura basmıştım; sürekli onu gösteriyor. Bu bug'ı önle artık."

Kök neden: 2.12/2.13 döneminde her "yeni tasarım oluştur" basışı proje
klasörüne yeni bir `FormN.m2god` bırakıyordu ve editör, her bağlanışta
`ListDesignFiles` ile hepsini bulup **her seferinde** seçim diyaloğu
çıkarıyordu. Seçimin hatırlanması diye bir mekanizma yoktu.

## Çözüm — 5 parça

| # | Değişiklik | Nerede |
|---|-----------|--------|
| 1 | **Proje hafızası** `.m2god_last`: bağlı proje klasörüne SON kullanılan tasarımın adı yazılır. Yeniden bağlanınca diyalog **çıkmasız**, tasarım otomatik yüklenir; durum çubuğu "değiştirmek için Proje → Tasarım Değiştir…" der | `designmem.cpp` + `DoPickDesignForProject` |
| 2 | **Proje → Tasarım Değiştir… (Ctrl+Shift+D)**: otomatik yükleme kullanıcıyı kilitlemesin diye seçici diyalog her zaman elle açılabilir | `DoChangeDesignDialog`, menü, hızlandırıcı |
| 3 | **İstatistikli tasarım listesi**: "Form2.m2god — 3 form, 21 obje, Modern". BOŞ tasarımlar (0 obje — 2.12/2.13 çöpü) "**BOŞ tasarım (0 obje) — çöp**" olarak etiketlenir | `designmem::Describe` / `DescribeLine` |
| 4 | **BOŞ TASARIMLARI SİL (çöp temizliği)** düğmesi: tek tuşla tüm boş tasarımları siler (onaylı), liste canlı yenilenir; hepsi boşsa taze Form1 açar | `ShowDesignOptionsDialog` |
| 5 | **Oturum hafızası** `%APPDATA%\M2GodWin32\last_session.txt`: editörü kapatıp açınca giriş ekranında **"SON TASARIMLA DEVAM ET — <dosya>"** düğmesi (ilk açılıştaki "eski menü" karışıklığı biter) | `designmem::RememberSession/LastSession`, `ShowWelcomeDialog` |

Ayrıca: tasarım listesi artık **gerçekten en yeniden eskiye** sıralanır
(eskiden alfabetikti — "varsayılan: en yeni" aslında çalışmıyordu).

## Yeni/Güncellenen Dosyalar

- `src/designmem.h` / `src/designmem.cpp` — **YENİ**: hafıza katmanı
  (istatistik + `.m2god_last` + oturum). mainwnd.cpp monolitinden bilinçli
  olarak ayrı → `tests/ptr_test16.cpp` ile doğrudan test edilebilir.
- `src/mainwnd.cpp` — diyalog yeniden düzenlemesi + otomatik yükleme akışı
  (`DoPickDesignForProject`), yeni menü/hızlandırıcı/komut, hoş geldin
  ekranı düğmesi, sürüm dizgeleri (başlık çubuğu "Faz 2.16").
- `tests/ptr_test16.cpp` — **YENİ**: 11 doğrulamalı birim testi.
- `M2GodWin32.vcxproj(.filters)` — designmem eklendi (VS2019'da açılır).
- `build_mingw.sh` — designmem + `Bin/M2GodWin32_Faz2.16.exe` çıktısı.

## Geriye Dönük Uyumluluk

- Eski `m2god-form` v2 (tek formlu) dosyalar `designmem::Describe` içinde
  desteklenir → listede doğru istatistik gösterilir, normal yüklenir.
- `.m2god_last` yoksa / silinmişse / kurcalanmışsa (yol ayraçlı değer)
  güvenle yok sayılır → eski diyalog akışı devreye girer (asla çökmez,
  yol enjeksiyonu engellenir).
- Salt-okunur proje klasöründe işaret yazılamazsa da her şey eskisi gibi
  çalışır (sadece hatırlama devre dışı kalır).

## Doğrulama (bu derlemede)

- `ptr_test16.exe`: **11 PASS / 0 FAIL** (Wine altında çalıştırıldı)
  — T1 dolu v3 istatistiği, T2 boş tasarım tespiti, T3 eski v2 uyumu,
  T4 bozuk dosya dayanıklılığı, T5 liste satırları, T6 işaret
  gidiş-dönüşü, T7 kurcalanmış işaret reddi, T8 işaret yoksa fallback.
- Editörün tamamı Wine+Xvfb altında çalıştırıldı; giriş ekranı ekran
  görüntüsü VLM ile doğrulandı: "SON TASARIMLA DEVAM ET — DenemeForm.m2god"
  düğmesi ve "SON seçilen tasarım OTOMATİK yüklenir" açıklaması görünüyor.
- llvm-mingw (UCRT) + VS2019 vcxproj derlemeleri temiz.

## Derleme

- **VS2019**: `M2GodWin32.vcxproj` → aç, derle (designmem zaten ekli).
- **Linux cross**: `bash build_mingw.sh` → `Bin/M2GodWin32_Faz2.16.exe`
  (llvm-mingw 20231114-ucrt).

## 2.13'e Göre Değişmeyen Her Şey

Tema sistemi (7 tema), klasör projesi (Forms.info), kod üretimi, VS
senkronizasyonu, tüm form etkileşimi — aynen korundu. Yama diyalog ve
hafıza katmanına odaklıdır; 2.14/2.15'te yaptığınız yerel değişiklikler
varsa aynı parçaları o ağaçlara taşıyabilirsiniz (tüm değişiklikler
`designmem.*` + `mainwnd.cpp` + vcxproj'ta).
