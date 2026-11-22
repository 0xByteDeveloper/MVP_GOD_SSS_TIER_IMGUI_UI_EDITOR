# IMGUI GOD SSS TIER — Teslim Paketleri (Sürüm 6)

## 1. IMGUI_GOD_SSSTIER.zip — Editör + kaynak kod (tam proje)

Dear ImGui tabanlı UI editör + VS2019 kod üreticisi (M2GodWin32 Faz 2.13'ün ImGui portu).

- `god_editor/src/` — editör kaynak kodu (canvas, model, katalog, kod üretici, özellik paneli, tema, objstyle)
- `god_editor/imgui/` — Dear ImGui 1.91.8 SDK + backend'ler (dx9/10/11/12, opengl3, win32)
- `god_editor/tests/` — **14 senaryo / 89 doğrulama** (Wine altında gerçek WM mesaj enjeksiyonu — hepsi PASS)
- `god_editor/Bin/imgod_editor_x64.exe` — DERLENMİŞ editör (mingw-w64, x64, statik) + `Bin/fonts/` (Türkçe DejaVu)
- `god_editor/build.sh` — bağımsız derleme (x86_64-w64-mingw32-g++-posix)
- `god_editor/README.md` — teknik özet

### Sürüm 6 (6. tur) — kullanıcının son raporunun tam çözümü
- **TextBox DİKEY BOYUT**: "sağa sola büyütüp küçültebiliyorum da yukarı aşağı yapamıyorum" —
  tek-satır çerçeve ailesi (TextBox/InputText, InputInt, InputFloat, Combo, SliderFloat/Int,
  DragFloat/Int) artık TAM boyut (W+H): yükseklik FramePadding ölçeklemesiyle çerçeve satırını
  büyütür; alt/üst tutamaçlar + NS imleçleri; özellik panelinde Boyut (w,h). Canvas VE üretilen
  IGOD sarmalayıcıları AYNI dönüşümü uygular (tam WYSIWYG, PushFrameHeight SDK yardımcısı).
- **GroupBox'A SÜRÜKLE-BIRAK İÇİNE KOYMA**: "nesneleri groupboxun içine koyamıyorum...
  nesneler onun içinde değilmiş" — nesne konteyner içine BIRAKILINCA onun ÇOCUĞU olur
  (WinForms/M2God davranışı): konteyneri taşıyınca içindekiler DE taşır; TabBar'a bırakılan
  aktif sekmeye bağlanır; DIŞINA bırakılınca forma döner. Özellik panelinde "Ebeveyn" satırı.
- **GROUPBOX KÜÇÜLÜNCE ÇOCUK KIRPILIR**: "groupboxu küçülttüğümde buttonu hâlâ görmeye devam
  ediyorum" — konteyner çocukları konteyner yüzeyine kırpılır (çizim + hit-test); küçülen
  groupbox'ın dışına taşan çocuk görünmez ve tıklanamaz; parent KORUNUR (büyütünce geri gelir).
- **YAN METİNLER KAPANABİLİR**: "textboxu oluşturuyom yanında string ile 'Yazı', checkboxda da
  'Seçenek'... bunu bi ayar ile kapatabilelim" — **Nesne Etiketleri (yan metinler)** ayarı:
  Görünüm menüsü + araç çubuğu düğmesi ("Etiketler: AÇIK / GİZLİ"). Kapatılınca denetimlerin
  YANINDAKİ metinler kalkar — canvas VE üretilen kod aynı şekilde etiketsiz çizer
  (u8"##ad" kimliği → ID çakışması yok). İçerik metinleri (düğme yazısı, Text, başlıklar) kalır.
- **PROAKTİF GİZLİ HATA AVLARI** ("başka sorun ve eksik varsa tespit et çöz"):
  - Araç çubuğu düğmeleri görünmezdi (SameLine+Separator satır kırılması → Geri/Sil/Üret
    kesiliyordu) → ToolbarSep ile TEK satıra düzeltildi.
  - Hit-test "en derin kazanır" hatası: konteyner ÇOCUĞU, üzerine çizilmiş form-seviyesi
    nesneden tıklamayı çalıyordu → gerçek görsel z-sırası (konteyner hit dikdörtgeleri
    çocuklarından ÖNCE kayıtlanır; "son çizilen kazanır").
  - GroupBox çerçevesi/başlığı SON ÇOCUĞUN yerine çiziliyordu (canvas + üretilen Global.h
    aynı hatayla) → child penceresinin kendi dikdörtgeninden (WYSIWYG düzeltme).
  - Özellik panelinde yükseklik "0" gösterimi (çerçeve ailesi/Checkbox) giderildi;
    katalogda Height/Width özellikleri tamamlandı.

## 2. DenemImgui.zip — üretilmiş ÖRNEK proje (derlenmiş)

Editörden üretilmiş 6. tur vitrin projesi: GroupBox "Giriş Alanı" (içinde DİKEY TextBox
"Merhaba ImGui", "Gönder" düğmesi, "Beni hatırla" checkbox'u), Panel (içinde DİKEY "Hız"
slider'ı + "Seçenek 1" combo'su), "Not" çok-satır alanı, ilerleme çubuğu, W×H ayraç,
"Temizle" düğmesi — konteyner çocuk hiyerarşisi + dikey boyutlar ÜRETİM KODUNDA birebir.

- `DenemImgui_final/DenemImgui/` — VS2019 çözümü (`.sln` + `.vcxproj`) + `build_mingw.sh`
- `DenemImgui_final/DenemImgui/Bin/DenemImgui.exe` — MinGW ile DERLENMİŞ çalışır exe
- `Form1.cpp/.h`, `Global.h` (IGOD SDK — PushFrameHeight dahil), `main.cpp`, `imgui/`, `fonts/`

Derleme: `cd DenemImgui && bash build_mingw.sh` (VS2019'da `.sln` açılır) — Wine/Linux'ta
`Bin/DenemImgui.exe` doğrudan çalışır (dx11).
