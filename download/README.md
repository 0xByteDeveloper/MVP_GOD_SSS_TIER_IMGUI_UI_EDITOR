# IMGUI GOD SSS TIER — Teslim Paketleri (Sürüm 4)

## 1. IMGUI_GOD_SSSTIER.zip — Editör + kaynak kod (tam proje)

Dear ImGui tabanlı UI editör + VS2019 kod üreticisi (M2GodWin32 Faz 2.13'ün ImGui portu).

- `src/` — editör kaynak kodu (canvas, model, katalog, kod üretici, özellik paneli, tema, objstyle)
- `imgui/` — Dear ImGui 1.91.8 SDK
- `tests/` — 10 senaryo / 45 doğrulama (Wine altında gerçek WM mesaj enjeksiyonu ile — hepsi PASS)
- `Bin/imgod_editor_x64.exe` — DERLENMİŞ editör (mingw-w64, x64, statik) + `Bin/fonts/` (Türkçe DejaVu)
- `build.sh` — bağımsız derleme (x86_64-w64-mingw32-g++)

Sürüm 4 yenilikleri — "combo/listbox GÖRSEL boyutlanmıyordu, yalnız özellikteki sayı değişiyordu"
raporunun KÖKTEN çözümü:

- **Combo, ListBox, Slider, Drag, InputText, InputInt/Float, ColorEdit4, Table artık
  GERÇEKÇEN boyutlanıyor** — SetNextItemWidth / BeginListBox(w,h) / outer_size hem editör
  canvas'ta hem üretilen kodda (IGOD `Width`/`Size` alanları) aynı değerle uygulanır.
- **Tipe göre dürüst tutamaç maskesi**: tam boyutlanan (Button/ProgressBar/Plot/Multiline/
  ListBox/Panel/Child/GroupBox) → 8 tutamaç; yalnız genişlik (Combo/Slider/Drag/Input/
  Color/Table) → sağ/sol kenar; içerik boyutlu (Checkbox/Radio/Text/Separator/Header/
  TreeNode/TabBar) → tutamaç yok + özellik panelinde "(otomatik)" — hayalet boyut yok.
- **Özellik paneli Boyut satırı yeteneğe göre**: FULL → `w, h` düzenlenebilir; W →
  `Genişlik` + yükseklik "(otomatik)"; AUTO → "(otomatik — içerikten ölçülür)".
- Tutamaçla boyut değişince Width/Height/SizeArg/HeightItems özellikleri AYNI sayıya
  senkron; ListBox yüksekliği ↔ HeightItems iki yön tutarlı.
- Üretilen `IGOD::ColorEdit4` editörle AYNI NoInputs kipini bağlı (WYSIWYG).

Sürüm 3: form boyutlandırma sınırı · tek yerleştirme · SAĞ TIK bağlam menüsü ·
Panel aracı · form başlığı nötr · gizli bellek/çöküş hataları.

## 2. DenemImgui.zip — üretilmiş ÖRNEK proje (derlenmiş)

Editörden üretilmiş, VS2019 ile açılabilir (`DenemImgui.sln`, v142 x64/Win32) ve
MinGW ile derlenebilir (`build_mingw.sh`) örnek ImGui uygulaması.

- `Form1.cpp/.h`, `Global.h` (IGOD SDK), `main.cpp` (DX11 + seçilebilir motorlar)
- Panel içi buton, checkbox, **280px slider**, **300px combo**, **240x110 listbox**,
  progress bar, Türkçe metinler — `combo1.Width = 300;` / `listBox1.Size = ImVec2(240, 110);`
  bağlamaları görünür
- `Bin/DenemImgui.exe` — DERLENMİŞ çalışır uygulama (mingw-w64, statik)
- `imgui/` + `fonts/` SDK kopyaları

Çalıştırma: `DenemImgui.exe` — editörde tasarlanan form AYNEN açılır (WYSIWYG doğrulandı:
geniş combo, boyutlu listbox derlenmiş uygulamada birebir aynı).

Kaynak: https://github.com/0xByteDeveloper/MVP_GOD_SSS_TIER_IMGUI_UI_EDITOR
