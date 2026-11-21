# IMGUI GOD SSS TIER — Teslim Paketleri (Sürüm 3)

## 1. IMGUI_GOD_SSSTIER.zip — Editör + kaynak kod (tam proje)

Dear ImGui tabanlı UI editör + VS2019 kod üreticisi (M2GodWin32 Faz 2.13'ün ImGui portu).

- `src/` — editör kaynak kodu (canvas, model, katalog, kod üretici, özellik paneli, tema, objstyle)
- `imgui/` — Dear ImGui 1.91.8 SDK
- `tests/` — 9 senaryo / 36 doğrulama (Wine altında gerçek WM mesaj enjeksiyonu ile — hepsi PASS)
- `Bin/imgod_editor_x64.exe` — DERLENMİŞ editör (mingw-w64, x64, statik) + `Bin/fonts/` (Türkçe DejaVu)
- `build.sh` — bağımsız derleme (x86_64-w64-mingw32-g++)

Sürüm 3 yenilikleri: form boyutlandırma sınırı düzeltildi (sınırsız büyüme bitti) ·
tek yerleştirme (araç seçimi otomatik kalkar) · SAĞ TIK bağlam menüsü (Kes/Kopyala/
Yapıştır/Klonla/Sil/Öne Getir/Arkaya Gönder/Özellikler) · yeni **Panel** aracı
(ImGui BeginChild tabanlı konteyner) · form başlığı nötr · gizli bellek/çöküş hataları giderildi.

## 2. DenemImgui.zip — üretilmiş ÖRNEK proje (derlenmiş)

Editörden üretilmiş, VS2019 ile açılabilir (`DenemImgui.sln`, v142 x64/Win32) ve
MinGW ile derlenebilir (`build_mingw.sh`) örnek ImGui uygulaması.

- `Form1.cpp/.h`, `Global.h` (IGOD SDK), `main.cpp` (DX11 + seçilebilir motorlar)
- Panel içi buton, checkbox, slider, combo, progress bar, Türkçe metinler
- `Bin/DenemImgui.exe` — DERLENMİŞ çalışır uygulama (mingw-w64, statik)
- `imgui/` + `fonts/` SDK kopyaları

Çalıştırma: `DenemImgui.exe` — editörde tasarlanan form AYNEN açılır (WYSIWYG doğrulandı).

Kaynak: https://github.com/0xByteDeveloper/MVP_GOD_SSS_TIER_IMGUI_UI_EDITOR
