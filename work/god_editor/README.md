# IMGUI GOD SSS TIER — ImGui UI Editör & Kod Üreticisi

**M2GodWin32 Faz 2.13 "Win32 API UI editörü + VS2019 kod üreticisi" projesinin TAM İMGLUI portu.**
Editörün KENDİSİ de ImGui'dir; üretilen formlar da ImGui'dir — **editörde gördüğün = derlenince
göreceğin (birebir WYSIWYG).**

> Win32'e özel temalar (Ruby/Yellow/Purple Graphite, comctl32, WM_CTLCOLOR) **bilinçli olarak YOK** —
> ImGui evreninde ImGui'nin kendi tema seti + 3 GOD SSS TIER askeri paleti geçerlidir.

---

## Öne çıkan özellikler

1. **Editör = ImGui**: RAD Studio düzeni — sol üst **Kod Gezgini** (hiyerarşik ağaç + bağlam menüsü),
   sol alt **Araç Kutusu** (kategoriler HER ZAMAN AÇIK, arama, 25 nesne), merkez **Tasarım Yüzeyi**
   (ızgara + seçim tutamaçları + bileşen tepsisi), sağ üst **Formlar**, sağ alt
   **Özellikler/Olaylar** (kategorik property grid).
2. **Birebir WYSIWYG**: tasarım yüzeyi, ÜRETİM temasıyla çizilir (`theme::PushProduction`) ve her
   nesne kendi `PushStyleColor/PushStyleVar` dizisiyle render edilir — üretilen `.cpp` AYNI diziyi
   kullanır. Derlenmiş uygulama = editör görünümü.
3. **Görsel nesne düzenleme**: tık-seç, Ctrl/Shift+tık çoklu seçim, **rubber band** (formda basılı
   tut + sürükle = toplu seçim), grup taşıma, ok tuşlarıyla ince ayar (Shift = ızgara adımı),
   8 yönlü boyutlandırma + **imleç geri bildirimi** (NWSE/NS/NESW/EW).
4. **Araç kutusu M2God akışı**: nesneye tıkla → seçili KALIR → formda tıkla → **TEK SEFER** yerleşir
   (yerleşince araç seçimi KALKAR — sınırsız oluşturma yok; yeni nesne için yeniden seç).
   **ESC / sağ tık** → iptal. **Çift tıkla** nesneyi aktif forma direkt ekler.
5. **SAĞ TIK BAĞLAM MENÜSÜ (canvas)**: nesne üstünde Kes / Kopyala / Yapıştır / Klonla / Sil /
   **Öne Getir** / **Arkaya Gönder** / Özellikler; form boşluğunda Yapıştır / Tümünü Seç /
   Özellikler. Sağ tık tıklanan nesneyi seçer (VS davranışı); menü açıkken kısayollar menüye
   karışmaz; ESC menüyü kapatır, seçimi KORUR.
6. **PANEL (yeni araç)**: WinForms Panel karşılığı — `ImGui::BeginChild` tabanlı kenarlıklı,
   içine nesne yerleştirilebilir taşıyıcı (`IGOD::Panel` olarak üretilir; `Rounding`, `BgColor`,
   `BorderColor` destekli). Konteynerler: **Panel**, ChildWindow, GroupBox, CollapsingHeader,
   TabBar, TreeNode, Table, Popup.
7. **Pano (M2God davranışı)**: Ctrl+C/X/V — yapıştırılan nesneler **TİP SAYACIYLA YENİ AD** alır:
   `denemebutton` kopyala → yapıştır → `button2`, `denemecombobox` → `combo2`.
8. **NESNE BAŞINA RENK/STİL (ImGui SDK özellikleri)**: her nesne için `TextColor`, `ButtonColor`,
   `ButtonHoverColor`, `ButtonActiveColor`, `FrameBgColor`, `CheckMarkColor`, `SliderGrabColor`,
   `ProgressColor`, `PlotColor`, `BgColor`, `BorderColor`, `TabSelectedColor`, `HeaderBgColor`,
   `SeparatorColor` (#RRGGBB; boş = tema kalıtımı) + `FrameRounding`, `FrameBorderSize`, `Alpha`,
   `Disabled`, `Repeat`, `Password`, `ReadOnly`. Form düzeyinde: `WindowBgColor`,
   `WindowRounding`, `WindowBorderSize`. Değerler `objstyle.cpp` üzerinden canvas VE üretilen
   koda AYNI dizide taşınır.
9. **İmgesiz ID garantisi**: her nesne etiketi `"metin##nesneAdı"` → ImGui ID çakışması imkânsız
   (aynı metinli iki düğme bile).
10. **5 grafik arka uç**: DX11 (varsayılan) / DX9 / DX10 / DX12 / OpenGL3 — editör VE üretilen
   uygulama. `--backend dx9` komut satırı veya F6 üretim diyaloğu.
11. **Üretim**: VS2019 çözümü (v142, x64+Win32) + MinGW derleme betiği + ImGui SDK + DejaVu
   fontları (Türkçe: ğ ş ı İ ö ü ç) + her form `<Ad>.cpp/.h` + boş gövdeli olay işleyiciler.
12. **Çok formlu + park/unpark**: formlar arası geçişte model JSON ile güvenle saklanır/geri yüklenir.

## Sürüm 3 — 3. tur kullanıcı raporlarına göre düzeltilenler

| Rapor | Çözüm |
|---|---|
| Form boyutlandırma SINIR TANIMADAN büyüyordu (her kare birikiyordu) | Boyut artık tutamaç yakalanırkenki boyuttan + KESİN delta ile hesaplanır; form canvas alanından dışarı taşıamaz (sol/üst tutamaç yok — yalnız sağ/alt büyütür) |
| Yerleştirmede "focus kalkmıyor, her tıkta sınırsız nesne oluşuyor" | TEK yerleştirme: form tık → nesne oluşur → araç seçimi otomatik KALKAR (M2God/VS davranışı); ESC veya sağ tık iptal |
| "Sağ tık + seçenekler (Öne Getir gibi) var mı?" | VAR: canvas üzerinde tam bağlam menüsü (Kes/Kopyala/Yapıştır/Klonla/Sil/Öne Getir/Arkaya Gönder/Özellikler + form menüsü); menünün formun arkasında kalması z-sırası hatası da düzeltildi |
| "Araç kutusunda Panel var mı? ImGui SDK'da panel?" | VAR: "Panel" aracı eklendi (ImGui'da panel = BeginChild bölgesi) — Rounding/BgColor/BorderColor destekli, WYSIWYG + IGOD::Panel kod üretimi |
| Form başlığı tıklayınca maviye dönüyordu | Başlık çubuğu NÖTR sabitlendi (TitleBgActive = TitleBg) |
| Form ekranı kaplatıyordu | Görünür boyut canvas'a sığdırılır; taşarsa kırpılır + "gerçek boyut" göstergesi |
| Gizli hatalar (kullanıcının göremedikleri) | InputText tamponunun sınırsız bellek şişmesi · silinen nesne tampon kalıntıları · ebeveyn+çocuk birlikte seçilince çocuğun iki kez taşınması · form adlandırma popup'ında yazılan adın anında sıfırlanması · menü açıkken ESC'nin seçimi silmesi · çoklu seçimde özellik değişikliğinin aynı tiplere uygulanması · ok tuşu taşımlarında tekil geri al kaydı |

## Sürüm 2 — kullanıcı raporlarına göre düzeltilenler

| Rapor | Çözüm |
|---|---|
| Araç kutusu kapalı geliyordu | Kategoriler `DefaultOpen` + `ImGuiCond_Always` — daima açık |
| Araç kutusunda seçim kalıcı olmuyordu | Aynı karede canvas tıklaması arm'ı iptal ediyordu — akış yeniden yazıldı: arm → form tık → yerleştir, ESC → iptal, çift tık → direkt ekle |
| Nesneler soluk/saydam görünüyordu | Form artık ÜRETİM temasıyla çizilir (editör teması değil) + her nesnenin kendi stil push'ları — derlenen görünüm = editör görünümü |
| Form'a odaklanınca ekranı mavi selected kaplıyordu | Klavye-nav kapatıldı, form penceresi sabitlendi (`NoMove/NoResize/NoBringToFrontOnFocus`), canvas z-sırası `BringWindowToDisplayFront` ile garanti |
| Tıklama/seçim konumu kayıyordu | Hit-dikdörtgenleri `GetItemRectMin/Max` ile gerçek çizim konumundan toplanır; seçim çerçevesi daima gerçek konumda |
| "2 visible items with conflicting ID" | Tüm etiketler `metin##ad` — ID çakışması kökten imkânsız |
| Kod gezginde tıklama seçmiyordu | Selectable tabanlı sağlam seçim + TreeNode hiyerarşisi + sağ tık bağlam menüsü |
| Renk ayarı yoktu | Nesne başına 17 renk + stil özellikleri (Renk/Stil kategorileri) + form renkleri |
| Toplu seçim/sürükleme/kes-kopyala-yapıştır yoktu | Rubber band + grup taşıma + ok tuşları + Del/Ctrl+X/C/V — tip sayaçlı yeni adlarla |
| İmleç değişmiyordu | Boyutlandırma tutamaçlarında NWSE/NS/NESW/EW imleçleri |

## Derleme

```bash
./build.sh              # x64 (mingw-w64 GCC — build_mingw.sh derleyiciyi otomatik bulur)
./build.sh debug        # + geri izlemeli IM_ASSERT teşhisi
```

Gereksinim: mingw-w64 (`x86_64-w64-mingw32-g++`). Çıktı: `Bin/imgod_editor_x64.exe` (+ `Bin/fonts/`).

## Çalıştırma

```
imgod_editor_x64.exe                       # normal
imgod_editor_x64.exe --backend dx9         # editör motoru DX9 (dx10/dx11/opengl3 da olur)
imgod_editor_x64.exe --selftest a.txt      # otomasyon (aşağıda)
imgod_editor_x64.exe --open proje.imgod    # projeyle başla
```

## Testler (askeri düzey)

```bash
tests/run_tests.sh
```

9 senaryo / 36 doğrulama: temel ekleme-özellik-kayıt · arm-yerleştir-rubber band-pano · tip sayaçlı adlandırma · çok formlu park/unpark · TEK yerleştirme · form boyutlandırma kesin delta + canvas sınırı · sağ tık seçim/menü/ESC koruması · Panel içine yerleştirme ·
yeniden adlandırılmış nesnelerin TİP SAYACI adlarıyla yapıştırılması · çok formlu park/unpark.
Tüm etkileşimler gerçek WM mesajlarıyla (PostMessage) enjekte edilir — sahte değil.

## Üretilen proje düzeni

```
<klasör>/
  <Proje>.sln                    VS2019 (v142, x64 + Win32)
  <Proje>/Global.h               IGOD SDK (WinForms sözdizimi sarmalayıcıları + temalar + Timer)
  <Proje>/main.cpp               seçili backend + DejaVu font kurulumu (Türkçe destek)
  <Proje>/Form1.cpp/.h           editör tasarımını AYNEN çizen ImGui kodu
  <Proje>/imgui/...              SDK kopyası (backends/ alt yapısıyla)
  <Proje>/fonts/                 DejaVuSans.ttf + DejaVuSansMono.ttf
  <Proje>/build_mingw.sh         derleyiciyi otomatik bulan bağımsız derleme
```

Üretilen kodun API uyumluluğu: ImGui 1.76 → 1.93 arası aynı widget çağrıları (bu depo 1.91.8 ile test edildi).

## Dosyalar

```
src/            editör kaynak kodu (canvas, model, katalog, kod üretici, özellik paneli, tema, objstyle)
imgui/          Dear ImGui 1.91.8 SDK (flat — üretime backends/ yapısıyla kopyalanır)
fonts/          DejaVu (Türkçe karakter desteği)
tests/          selftest betikleri + run_tests.sh
Bin/            imgod_editor_x64.exe + fonts/
```

## Lisans

MIT — Dear ImGui (MIT) dahil.
