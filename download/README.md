# IMGUI GOD SSS TIER — Teslim Paketleri (Sürüm 5)

## 1. IMGUI_GOD_SSSTIER.zip — Editör + kaynak kod (tam proje)

Dear ImGui tabanlı UI editör + VS2019 kod üreticisi (M2GodWin32 Faz 2.13'ün ImGui portu).

- `god_editor/src/` — editör kaynak kodu (canvas, model, katalog, kod üretici, özellik paneli, tema, objstyle)
- `god_editor/imgui/` — Dear ImGui 1.91.8 SDK + backend'ler (dx9/10/11/12, opengl3, win32)
- `god_editor/tests/` — **12 senaryo / 70 doğrulama** (Wine altında gerçek WM mesaj enjeksiyonu — hepsi PASS)
- `god_editor/Bin/imgod_editor_x64.exe` — DERLENMİŞ editör (mingw-w64, x64, statik) + `Bin/fonts/` (Türkçe DejaVu)
- `god_editor/build.sh` — bağımsız derleme (x86_64-w64-mingw32-g++-posix)
- `god_editor/README.md` — teknik özet

### Sürüm 5 (5. tur) — kullanıcının son raporunun tam çözümü
- **TabBar W×H boyutlandırılabilir** (child bölge sarmalaması; canvas = üretilen kod).
- **GroupBox/Panel içine nesne KOYULABİLİR**: derinlik tabanlı hit-test — konteyner
  çocuklarını gölgelamiyor; içteki nesne tıklanır/seçilir/taşınır/boyutlanır. İç içe
  konteynerlerde en İÇTEKİ kazanır; TabBar'a yerleşim TabOwner=aktif sekme (nesne görünür).
- **LabelText genişliği kısaltılabilir** (değer sağ uçta hizalanır).
- **Text/TextColored/BulletText/Checkbox/RadioButton boyutlandırılabilir**: metin wrap,
  etiket kısaltma, checkbox kare yükseklikle büyür (WinForms ruhu; üretim kodu aynı dönüşüm).
- **Form boş alanına tıkla = form seçimi** (eskiden yalnız ESC seçiyordu).
- **TextBox + TextBoxMultiline** araç kutusunda (M2God sözlüğü).
- **Separator GERÇEK W×H dolgu çubuğu** (kalınlık ayarlanabilir); SeparatorText w'ye kadar
  çizgi; CollapsingHeader/TreeNode genişlik sınırı.
- **Araç kutusu arama ÖĞE bazlı** ("label" yazınca yalnız eşleşen nesneler listelenir).
- **Editör kroması NÖTR GRI** (her temada): başlıklar/sekmeler/giriş kutuları/düğmeler mavi
  değil; form listesi seçili satır koyu gri; + Yeni Form yeşil / Sil kırmızı / Yen. Ad nötr;
  canvas zemini + ızgara + temizlik rengi nötr. Tasarım formu WYSIWYG üretim temasında.
- **Kod üretimi sağlamlaştırma**: Form1.h çocuk ÇİFT bildirim hatası giderildi; üretilen
  proje MinGW ile DERLENİP doğrulanıyor (IGOD SDK: FitTextToWidth, boyutlu
  Separator/SeparatorText/BulletText, Checkbox/Radio W+H, TabBar Size).

## 2. DenemImgui.zip — üretilmiş ÖRNEK proje (derlenmiş)

Editörden üretilmiş vitrin projesi: Panel (içinde buton), TextBox, LabelText, kaydırmalı Text,
büyük Checkbox, RadioButton, W×H Separator, SeparatorText, BulletText, sağ sütun klasikleri
(Slider/Combo/ListBox/ProgressBar), W×H TabBar (içinde buton), GroupBox (içinde TextBox).

- `DenemImgui_final/DenemImgui/` — VS2019 çözümü (`.sln` + `.vcxproj`) + `build_mingw.sh`
- `DenemImgui_final/DenemImgui/Bin/DenemImgui.exe` — MinGW ile DERLENMİŞ çalışır exe
- `Form1.cpp/.h`, `Global.h` (IGOD SDK), `main.cpp`, `imgui/`, `fonts/`

Derleme: `cd DenemImgui && bash build_mingw.sh` (VS2019'da `.sln` açılır) — Wine/Linux'ta
`Bin/DenemImgui.exe` doğrudan çalışır (dx11).
