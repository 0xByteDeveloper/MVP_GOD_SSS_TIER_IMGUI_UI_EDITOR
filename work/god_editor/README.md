# IMGUI GOD SSS TIER — UI Editör ve Kod Üreticisi (v1.6 — 6. Tur Düzeltmeleri)

M2GodWin32 Faz 2.13 (Win32 UI editör + VS2019 kod üreticisi) evreninin **saf Dear ImGui** dönüşümü.
Editör tamamen ImGui ile çizilir; ürettiği proje VS2019 (v142) VEYA MinGW ile derlenir
(DX9/DX10/DX11/DX12/OpenGL3 motor seçimiyle).

## Çalıştırma
- `Bin/imgod_editor_x64.exe` — hazır derlenmiş x64 editör (DX11; `--backend dx9|dx10|opengl3` ile değişir)
- Kaynaktan: `./build.sh` (mingw-w64 x86_64-w64-mingw32-g++-posix gerektirir) → `Bin/imgod_editor_x64.exe`

## Test paketi
`cd tests && ./run_tests.sh` (Wine + Xvfb altında; 14 senaryo / 89 doğrulama, hepsi PASS)

## 6. TUR DÜZELTMELERİ (kullanıcı raporları)
1. **"TextBox'u sağa-sola büyütüp küçültebiliyorum ama yukarı-aşağı yapamıyorum"** — ÇÖZÜLDÜ:
   tek-satır çerçeve ailesi (TextBox/InputText, InputInt, InputFloat, Combo, SliderFloat/Int,
   DragFloat/Int) artık TAM boyut (W+H). Yükseklik FramePadding ölçeklemesiyle çerçeve satırını
   büyütür — canvas VE üretilen IGOD sarmalayıcıları AYNI dönüşümü uygular (WYSIWYG).
   Alt/üst/kenar tutamaçları + NS imleçleri çalışır; özellik panelinde Boyut (w,h) düzenlenir.
2. **"Nesneleri groupbox'un içine koyamıyorum; koyduğumda nesneler onun içinde değilmiş"** — ÇÖZÜLDÜ:
   SÜRÜKLE-BIRAK RE-PARENT. Nesne bir konteynerin (Panel/ChildWindow/GroupBox/TabBar) içine
   bırakılınca onun ÇOCUĞU olur — WinForms/M2God tasarımcı davranışı:
   - Konteyner taşınır → içindekiler DE hareket eder (parent-göreli koordinat).
   - TabBar'a bırakılan nesne AKTİF sekmenin çocuğu olur (TabOwner otomatik).
   - Konteyner DIŞINA bırakılınca FORMA döner (koordinat çevrimi otomatik).
   - Özellik panelinde "Ebeveyn" satırı görünürlüğü (hangi konteynerin içinde olduğu belli).
   - Tek-tık yerleştirmede en İÇTEKİ konteyner hedeflenir (GroupBox başlık alanı dahil tıklanır).
3. **"Groupbox'u küçülttüğümde buttonu hâlâ görmeye devam ediyorum"** — ÇÖZÜLDÜ:
   konteyner çocukları artık konteyner yüzeyine KIRPILIR (BeginChild klip yüzeyi — hem çizim hem
   hit-test). Küçülen groupbox'ın dışına taşan çocuk görünmez VE tıklanamaz (rubber band da
   yakalamaz). Parent ilişkisi KORUNUR (tekrar büyütünce çocuk geri görünür).
4. **"Yanındaki stringleri (Yazı/Seçenek) bir ayar ile kapatabileyim"** — ÇÖZÜLDÜ:
   **Nesne Etiketleri (yan metinler)** ayarı: Görünüm menüsü + araç çubuğu düğmesi
   ("Etiketler: AÇIK/GİZLİ"). Kapatılınca denetimlerin YANINDAKİ metinler kalkar — canvas VE
   üretilen kod aynı şekilde etiketsiz çizer (u8"##ad" kimliği → ID çakışması yok).
   İçerik metinleri (düğme yazısı, Text, başlıklar, sekme adları) KORUNUR.
5. GİZLİ HATALAR (proaktif tespit — "başka sorun ve eksik varsa tespit et çöz"):
   - **Araç çubuğu satır-kırılma hatası**: `SameLine()+Separator()` kombinasyonu düğmeleri satırlara
     sarıp 30px'lik araç çubuğundan taşıyordu — Geri/İleri/Sil/Üret + durum metni GÖRÜNMEZDİ.
     Yeni ToolbarSep: dikey çizgi draw-list + Dummy yer tutucu → tüm araç çubuğu TEK satırda.
   - **Z-sırası (hit-test) hatası**: "en derin kazanır" kuralı, konteyner çocuğunun, ÜZERİNE
     çizilmiş form-seviyesi nesneden tıklamayı ÇALMASINA yol açıyordu. Konteyner hit
     dikdörtgenleri artık çocuklarından ÖNCE kayıtlanır → "son çizilen kazanır" = GERÇEK görsel
     z-sırası (WinForms davranışı).
   - **GroupBox çerçeve/başlık kayması (canvas + üretilen kod)**: çocuklar çizildikten SONRA
     GetItemRectMin alınıyordu → SON ÇOCUĞUN dikdörtgeni geliyordu: çerçeve yanlış yerde, başlık
     son çocuğun ÜZERİNE çizilirdi. Artık child penceresinin KENDİ dikdörtgeninden alınır
     (canvas'ta ImGuiWindow::Rect, üretilen Global.h'de GetWindowPos/GetWindowSize — WYSIWYG).
   - Özellik paneli: çerçeve ailesi/Checkbox/Radio için yükseklik "0" gösterimi kaldırıldı
     (gerçek çizim yüksekliğiyle senkron); katalogda Height/Width özellikleri eksiksiz.

## Önceki turların özü (hatırlatma)
- 5. tur: TabBar/Text/Checkbox/Separator boyutları · derinlik hit-test · TextBox araç kutusunda ·
  araç kutusu ÖĞE bazlı arama · nötr gri editör kroması · ListBox/Combo görsel boyut.
- Önceki: tek yerleştirme + ESC iptal · çoklu seçim/rubber band/ok tuşları/Del/Ctrl+XCV (yeni
  adlar TİP sayaçlı) · sağ tık menüsü (Öne Getir/Arkaya Gönder) · form min/max sınırları ·
  tutamaç imleçleri · WYSIWYG üretim teması · renk/stil özellikleri (objstyle) · .imgod JSON
  kayıt/yükleme · çok form park/unpark · VS2019 .sln/.vcxproj + MinGW build_mingw.sh ·
  3 özel tema (GOD OPS / Midnight Steel / Ammo Tan; Ruby Graphite vb. Win32 temaları YOK).

## Klasörler
- `src/` — editör kaynakları (canvas, model, katalog, codegen, tema, paneller…)
- `imgui/` — Dear ImGui 1.91.8 + backend'ler (win32, dx9/10/11/12, opengl3)
- `tests/` — 15 selftest senaryosu + run_tests.sh (89 doğrulama)
- `fonts/` — DejaVu (Türkçe karakter desteği)
