# IMGUI GOD SSS TIER — UI Editör ve Kod Üreticisi (v1.5 — 5. Tur Düzeltmeleri)

M2GodWin32 Faz 2.13 (Win32 UI editör + VS2019 kod üreticisi) evreninin **saf Dear ImGui** dönüşümü.
Editör tamamen ImGui ile çizilir; ürettiği proje VS2019 (v142) VEYA MinGW ile derlenir
(DX9/DX10/DX11/DX12/OpenGL3 motor seçimiyle).

## Çalıştırma
- `Bin/imgod_editor_x64.exe` — hazır derlenmiş x64 editör (DX11; `--backend dx9|dx10|opengl3` ile değişir)
- Kaynaktan: `./build.sh` (mingw-w64 x86_64-w64-mingw32-g++-posix gerektirir) → `Bin/imgod_editor_x64.exe`

## Test paketi
`cd tests && ./run_tests.sh` (Wine + Xvfb altında; 12 senaryo / 70 doğrulama, hepsi PASS)

## 5. TUR DÜZELTMELERİ (kullanıcı raporları)
1. **TabBar boyutlandırılabilir** — W×H child bölgesi sarmalaması (canvas + üretilen kod aynı).
2. **GroupBox/Panel içine nesne** — derinlik tabanlı hit-test: konteyner artık ÇOCUKLARINI
   gölgelamiyor (içteki nesne tıklanır, seçilir, taşınır, boyutlanır). Yerleştirme en İÇTEKİ
   konteynere; TabBar içine yerleşimde TabOwner = aktif sekme (nesne artık kaybolmuyor).
3. **LabelText genişliği** — SetNextItemWidth ile değer sağ uçta hizalanır (kısaltılabilir).
4. **Text / TextColored / BulletText / Checkbox / RadioButton boyutlandırılabilir**:
   - Text ailesi: PushTextWrapPos ile w'de satır kırma (yükseklik içerikten).
   - BulletText: genişliğe sığdırma ("..." kısaltma) + kontrol alanı.
   - Checkbox/RadioButton: yükseklik FramePadding ölçeklemesiyle KAREYİ büyütür; genişlik
     etiket sığdırma + kontrol alanı (WinForms ruhu).
5. **Form boş alanına tıklama = form seçimi** (eski s_hostHovered hatası; ESC artık yalnız
   seçim temizler/iptal eder).
6. **TextBox + TextBoxMultiline** araç kutusunda (M2God sözlüğü; üretimde IGOD::InputText).
7. **Separator GERÇEK W×H dolgu çubuğu** (kalınlık ayarlanabilir); SeparatorText w'ye kadar çizgi;
   CollapsingHeader/TreeNode genişlik sınırı (AutoResizeY child-clip).
8. **Araç kutusu arama ÖĞE bazlı filtreler** ("label" → yalnız eşleşen nesneler görünür).
9. **Editör kroması nötr gri** (her temada): başlık/sekmeler/giriş kutuları/düğmeler mavi değil;
   form listesinde seçili satır koyu gri; + Yeni Form yeşil, Sil kırmızı, Yen. Ad nötr.
   Canvas zemini/ızgara/temizlik rengi de nötr. (Tasarım formu WYSIWYG üretim temasında kalır.)
10. **Kod üretimi sağlamlaştırması**: Form1.h'te çocukların ÇİFT bildirimi düzeltildi (konteyner
    çocukları + IGOD sarmalayıcıları); üretilen proje MinGW ile DERLENİP doğrulanıyor.
11. IGOD SDK: FitTextToWidth (UTF-8 güvenli kısaltma), boyutlu Separator/SeparatorText/BulletText,
    Checkbox/RadioButton Width/Height, TabBar Size, CollapsingHeader/TreeNode Width.

## Önceki turların özü (hatırlatma)
Tek yerleştirme + ESC iptal · çoklu seçim/rubber band/ok tuşları/Del/Ctrl+XCV (yeni adlar TİP
sayaçlı) · sağ tık menüsü (Öne Getir/Arkaya Gönder) · form min/max sınırları · tutamç imleçleri ·
WYSIWYG üretim teması · renk/stil özellikleri (objstyle) · .imgod JSON kayıt/yükleme ·
çok form park/unpark · VS2019 .sln/.vcxproj üretimi + MinGW build_mingw.sh · 3 özel tema
(GOD OPS / Midnight Steel / Ammo Tan; Ruby Graphite vb. Win32 temaları bilinçli olarak YOK).

## Klasörler
- `src/` — editör kaynakları (canvas, model, katalog, codegen, tema, paneller…)
- `imgui/` — Dear ImGui 1.91.8 + backend'ler (win32, dx9/10/11/12, opengl3)
- `tests/` — 12 selftest senaryosu + run_tests.sh (70 doğrulama)
- `fonts/` — DejaVu (Türkçe karakter desteği)
