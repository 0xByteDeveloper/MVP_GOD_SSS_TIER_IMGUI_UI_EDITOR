# M2GodWin32 — Faz 2.13

## Faz 2.13 — M2GOD'UN 4 TEMASI + KLASÖR PROJESİ (Forms.info)

1. **M2GOD'UN 4 TEMASI EKLENDİ (kullanıcı: "m2god un içinde ruby, yellow, purple, dark
   themeler mevcut — button falan herşeyi kapsasın"):** Faz2.5 programının 4 teması
   (`theme.h` paletleri birebir) editöre VE üretilen koda taşındı — Tema menüsünde 7 seçenek:
   - **Ruby Graphite** — 5 duraklı KIRMIZI gradyan butonlar (0.0/0.15/0.5/0.85/1.0 —
     Faz2.5 `btn_Paint` durakları birebir: FF6060→E00000→CC0000→990000→E00000),
     hover/basılmış gradyanları ayrı, 1px kenar, (+1,+1) alfa gölgeli beyaz metin
   - **Yellow Graphite** — ALTIN gradyan butonlar (EBDCA2→EAC046→E09E00→C78500→B27700)
   - **Purple Graphite** — mor vurgulu grafit (başlık gradyanı 60,15,30→40,10,20)
   - **Dark** — koyu grafit (606060→303030 degrade)
   - **HER ŞEYİ KAPSAR:** Button (5 duraklı gradyan), CheckBox/RadioButton (koyu grafit
     kutu + tema vurgusu + beyaz işaret), GroupBox (M2God karakter kutusu görünümü —
     yuvarlak #6E6E74 kenar + çizgi hizasında açık başlık), TextBox/ListBox (#141414
     koyu alan + 1px tema kenarı + beyaz metin — WM_CTLCOLOR), ComboBox
     (**DarkComboBox** görünümü: koyu gövde + tema renkli ok düğmesi gradyanı),
     form zemini **#252525** (CLR_FORM_BACK), başlık çubuğu **tema gradyanı**
     (Ruby: 118,3,0→53,1,0) + sağda tek X düğmesi (ana programdaki btnClose gibi)
   - **Üretilen kod da aynı görünümü taşır:** `GraphiteCtl::Apply(h, 3..6)` ile nesne
     özel tema; üretilen uygulamanın Tema menüsü 7 tema taşır (IDRub..IDDrk);
     koyu alanlar `HandleCtlColor` ile #141414 + beyaz metin; hepsi YERİNDE deri
     değişimi — çökmez

2. **KLASÖR PROJESİ (kullanıcı: "artık multiple form kullanabildiğimiz için Forms.info
   (içinde formların bilgileri olur kaç form olduğu) — kaç form varsa Form1.info,
   Form2.info gibi olur"):** yeni proje düzeni —
   - `Forms.info` → kaç form + ad listesi + proje teması + aktif form
   - `Form1.info`, `Form2.info`... → her formun tasarımı (.m2god'daki forms[i] ile
     aynı JSON yapısı)
   - **Dosya → Klasör Projesi Aç (Forms.info)... Ctrl+Shift+O** ve
     **Dosya → Klasör Projesi Kaydet... Ctrl+Shift+S**; Ctrl+O ile Forms.info seçilirse
     otomatik klasör projesi olarak yüklenir; Kaydet/otomatik kayıt klasör formatına gider
   - **Faz2.5 M2God projesi bu düzenle açılır** (zip içinde Forms.info + Form1.info +
     Form2.info — MainForm + Clientsettings1, Ruby teması)

3. **Faz2.5 projesi UI editöre uyarlandı:** MainForm (462×649, menü açık düzen) ve
   Clientsettings1 (671×479, 5 ana sekme + iç içe alt sekmeler) denetim tablolarından
   birebir çıkarıldı; .info dosyaları zip'e eklendi; ekran görüntüleri
   `UIEditor_Form1_Ruby.png` / `UIEditor_Form2_Ruby.png` / `UIEditor_Tema4lu_MainForm.png`.

4. Kod Gezgini/Araç Kutusu/Özellikler (sağ tık menüsü 8 tema seçeneği + M2GodTheme enum
   8 değer) — hepsi 7 temalı sistemle uyumlu.

---

# M2GodWin32 — Faz 2.13 God Mode MVP

**Visual Studio 2019 C++ CLR Form Designer deneyimini Win32 API ile yaşatan, tam kapsamlı görsel UI tasarım ortamı.**

> Bu proje `CppGUICreatorWin32Api` deposunun **Faz 2.13** geliştirme aşamasıdır.
> Katalog kaynağı: **Config_info.json** (144 eleman, tüm property'ler + event'ler + 203 enum) — exe yanına konur;
> bulunamazsa yerleşik katalog kullanılır.

---

## Faz 2.12 — TEMA ÇÖKMESİ KÖKTEN ÇÖZÜLDÜ + GERÇEK AQUA + RAD STUDIO SİLİNDİ

1. **"2 KEZ TEMA DEĞİŞİNCE ÇÖKİYOR" KÖKTEN ÇÖZÜLDÜ (mimari değişikliği):**
   Eski mimari comctl32 v5↔v6 **aktivasyon bağlamı** (CreateActCtx/ActivateActCtx) açıp kapar ve her
   tema geçişinde **TÜM panelleri + tüm tasarım denetimlerini İMHA edip yeniden yaratıyordu**. İki
   dayanaksız durum vardı: (a) comctl32 pencere **sınıfları süreçte bir kez kaydedilir** (ilk kaydeden
   kazanır) — bağlam aç/kapat yeni pencerelere v6 görünümü **veremezdi**; yalnızca yüzlerce bağlam
   geçişi ve pencere imha/yeniden yaratma döngüsü üretiyordu → 2. tema değişiminde erişim ihlali.
   (b) `theme::Effective("")` hatası boş `Theme` özelliğini "Oldschool" sanıyordu — Modern/Apple
   modunda **her denetim yaratımı** Push/Pop ile bağlamı kapatıp açıyordu.
   **FAZ 2.12 çözümü:** aktivasyon bağlamı ve panel imha/yeniden yaratma döngüsü **TAMAMEN
   KALDIRILDI**. Temalar artık **saf `paintctl` çizimidir**; tema geçişi = denetim **derilerini
   YERİNDE değiştirmek** (`paintctl::Apply` — skin güncelle/tak/sök). **Hiçbir pencere imha
   edilmez → kaç kez değiştirilirse değiştirilsin ÇÖKMEZ.** Ayrıca `Effective("")` hatası düzeltildi
   (boş Theme = kalıtsal → editör teması).

2. **APPLE & MAC GERÇEKTEN AQUA (kullanıcı: "aşırı az benziyor, buton falan değişmemiş"):**
   Butonların değişmemesinin asıl nedeni (1b)'deki `Effective("")` hatasıydı — düzeltildi ve
   paletler gerçek macOS görünümüne yükseltildi:
   - **KAPSÜL Aqua buton:** uçları **tam yuvarlak** (radius = yükseklik/2), beyaz→#E3E3E6 degrade +
     **parlak üst bant** (Aqua hissi), hover'da koyu kenar, odakta 2px mac mavisi halka; varsayılan
     buton = **mac sistem mavisi kapsül** (#4FA3FF→#0A64D2) + beyaz metin.
   - **Mac checkbox (Big Sur tarzı):** 16px yuvarlatılmış kutu, işaretli = **düz #0A7AFF** + 3px beyaz
     kalın onay; üç durumlu = beyaz çizgi; odak halkası.
   - **Mac radio:** beyaz daire + ince kenar; seçili = **#0A7AFF iç nokta**.
   - **Mac groupbox (NSBox):** yuvarlak ince #D5D5D8 çerçeve.
   - **Mac alan kenarı (YENİ):** TextBox/ListBox'un sistem 3D kenarı kaldırılıp **odakta mavi
     yuvarlak çerçeve** çizilir (macOS TextField görünümü) + metin 4px içeriden başlar.
   - Tasarım yüzeyi: **trafik ışıklı** başlık + **#ECECEC** mac zemin.

3. **MODERN TEMASI AYRINTILI .NET 4.8 WINFORMS GÖRÜNÜMÜ:** buton (hover açık mavi, basılı koyu mavi,
   noktalı odak işareti), checkbox/radio (#0078D7), WinForms groupbox (düz köşe), **düz alan
   çerçevesi** (odakta #0078D7), **#F0F0F0 WinForms Control zemin**.

4. **RAD STUDIO TEMASI SİLİNDİ (kullanıcı isteği: "RAD olmamış — sil temayı"):** menüden, enum'dan,
   kayıttan ve üretilen koddan tamamen çıkarıldı. Eski "RadStudio" kayıtlı projeler açılırken
   Oldschool'a düşer (geriye uyumlu).

5. **Nesne özel tema YERİNDE:** özellik panelindeki `Theme` satırı ve sağ tık menüsü değişince
   denetim **yeniden yaratılmaz** — yalnız onun derisi değişir. Seçenekler:
   `(Proje Varsayılanı) / Oldschool / Modern / Apple & Mac`.

6. **Üretilen uygulama da aynı mimari:** Global.h'de `M2God::CtlPaint::SetThemeAll(tema, form)`
   tüm denetim derilerini **EnumChildWindows ile yerinde** değiştirir; "Tema" menüsü 3 tema taşır
   (kayıt defteri 0..2), form zemini sınıf fırçası güncellenir — pencere imha/yeniden yaratma yok
   → üretilen exe'de de kaç tema geçişi yapılırsa yapılsın çökme yok. Nesne özel AppleMac denetimi
   `M2God::AquaCtl::Apply(handle)` ile **kilitli** deri alır (menüden tema değişse bile kalır).

---

## Faz 2.11 — APPLE & MAC TAM GÖRÜNÜM + RAD STUDIO TEMASI (TARİHSEL)

1. **APPLE & MAC ARTIK GERÇEKTEN MAC (kullanıcı geri bildirimi: "UI'da mac'e benzeyen
   değişiklik görmüyorum"):** Faz 2.10'da yalnız butonlar mac çiziliyordu; artık **çizim
   katmanı (`paintctl.cpp` / `M2God::CtlPaint`) tüm ana kontrolleri kaplar**:
   - **Aqua buton:** yusyuvarlak gövde, beyaz→gri degrade, hover koyulaşan kenar, mavi odak halkası,
     varsayılan buton mac mavisi dolgu + beyaz metin.
   - **Mac checkbox:** yuvarlatılmış kutu; işaretli = mavi dolgu (#007AFF) + beyaz onay işareti,
     üç durumlu = beyaz çizgi; odak = mavi halka.
   - **Mac radio:** mavi daire + beyaz merkez noktası; işaretsiz = beyaz + gri çerçeve.
   - **Mac groupbox:** yuvarlak ince gri çerçeve, başlık çerçeve üzerinde.
   - **Tasarım yüzeyi başlık çubuğu:** macOS **trafik ışıkları** (kırmızı/sarı/yeşil daireler),
     #ECECEC mac pencere zemini; sağda Windows kutucukları YOK (mac penceresinde yoktur).
   - Üretilen uygulamada aynı paletler `M2God::AquaCtl::Apply(handle)` ile taşınır.

2. **YENİ DÖRDÜNCÜ TEMA: RAD STUDIO (VCL tarzı) — kullanıcı onayıyla el yapımı:**
   - **VCL buton:** beyaz→gri degrade, 3px yuvarlatılmış köşe, hover'da açık mavi vurgu,
     basılıda koyu gri; varsayılan buton VCL mavisi çerçeve.
   - **VCL checkbox/radio:** mavi (#1082DE) dolgulu onay, mavi halka + mavi merkez nokta radio.
   - **VCL groupbox** + **#F0F0F0 VCL form zemini**.
   - **Tasarım yüzeyi başlık çubuğu:** koyu IDE slate (#3A404A), beyaz metin/glifler.
   - Üretilen uygulamada `M2God::RadCtl::Apply(handle)` ile taşınır.

3. **Tema menüsü artık 4 tema:** Oldschool / Modern / Apple & Mac / RAD Studio.
   Nesne özel `Theme` combobox ve sağ tık menüsü de 5 seçenek:
   `(Proje Varsayılanı) / Oldschool / Modern / Apple & Mac / RAD Studio`.

4. **Üretilen uygulama "Tema" menüsü 4 tema taşır** (kayıt defteri 0..3); nesne özel temalı
   kontroller tema geçişinde çizimlerini korur (AquaCtl/RadCtl yaratım bloğunda).

5. `.m2god` kaydı: `"Theme": "Oldschool" | "Modern" | "AppleMac" | "RadStudio"` —
   eski dosyalar geriye uyumlu açılır.

---

## Faz 2.10 — TEMA GEÇİŞ ÇÖKMESİ KÖKTEN DÜZELTİLDİ + APPLE & MAC TEMASI

1. **ÇÖKME DÜZELTMESİ (Modern → Oldschool geri dönüş):** Faz 2.9.1'deki "yok et → bağlam değiştir →
   yarat" sırası yetmedi; çökmenin asıl kökü **comctl32'nin `SetWindowSubclass`'ının v5/v6 karışık
   süreçte yanlış comctl sürümüne bağlanmasıydı**. Süreç Oldschool başladığında exe'nin comctl32
   içe aktarımları **v5'e** çözümlenir; Modern temada (comctl32 v6 bağlamı aktifken) yaratılan
   `SysListView32/SysTabControl32/SysTreeView32/msctls_*` tasarımcı kontrolleri İÇSELDE v6'nın
   kendi alt sınıf zincirini kullanır. Onlara v5'in `SetWindowSubclass`'ını uygulamak alt sınıf
   verisini bozuyor; pencere yok edilirken (tema geri dönüşünde) erişim ihlali → ÇÖKÜYORDU.
   **Düzeltme (her iki katmanda):**
   - `surface::SubclassForDesign` artık **saf kullanıcı32 alt sınıflama** (`SetWindowLongPtrW` +
     eski proc pencere özelliğinde `M2OLD_DESIGN`) — comctl32 sürümünden bağımsız, her temada güvenli.
   - Üretilen koddaki konteyner köprüsü de `SetWindowSubclass` yerine **`M2God::BridgeSubclass`**
     (Global.h v2100) — nesne özel temalı + çalışma anı tema geçişli üretilen uygulamalarda aynı
     risk yok.
   - Ek sağlamlaştırma: tema geçişinde **araç kutusu ağacı yeniden doldurulur** (2.9.1'de boş
     kalıyordu), yüzey `WM_CTLCOLOR` fırça önbelleği yok edilişte temizlenir (bayat HWND dönerse
     yanlış fırça dönmesin), `WM_DESTROY` kontrolleri `IsWindow` korumasıyla yok eder.

2. **APPLE & MAC TİPİ ÜÇÜNCÜ TEMA (kullanıcı isteği):** Tema menüsünde üç seçenek:
   **Oldschool / Modern / Apple & Mac (Aqua tarzı)**. Apple & Mac = comctl32 v6 tabanı +
   **mac özel buton çizimi**: yusyuvarlak gövde, beyaz→gri yumuşak geçiş, ince kenar, mavi odak
   halkası; tasarımda `BackColor=Control` olan form zemini aqua grisi (#F5F5F7) olur.
   - **Nesne özel tema genişletildi:** Özellik panelindeki `Theme` satırı ve sağ tık menüsü artık
     4 seçenekli combobox: `(Proje Varsayılanı) / Oldschool / Modern / Apple & Mac`.
   - **Üretilen kodda:** etkili teması Apple & Mac olan (global ya da nesne özel) **Button**
     kontrollerinin yaratımından sonra `M2God::MacStyle::Apply(button1->Handle)` yazılır;
     üretilen uygulamanın "Tema" menüsü de 3 tema taşır (kayıt defteri 0/1/2).
   - `.m2god` kaydı: `"Theme": "Oldschool" | "Modern" | "AppleMac"` (nesnelerinki `Props`
     sözlüğünde `AppleMac` olarak normalleştirilir; eski dosyalar geriye uyumlu açılır).

3. **Tema sistemine kimlik temelli API:** `theme.h/cpp` — `Switch/Push/Effective(int)`; ad↔kimlik
   dönüşümleri (`IdOfName/NameOf/SaveNameOf`); Modern↔Apple geçişinde bağlam zaten açıksa sadece
   kimlik değişir. `forms::ThemeId()` proje varsayılanını taşır. Global.h'de `M2God::Theme::Cur`
   (int) + **eski bool API geriye uyumlu aşırı yüklemeler** — eski üretilmiş Form1.cpp'ler yeni
   Global.h ile aynen derlenir (doğrulandı).

---

## Faz 2.9.1 — TEMA GEÇİŞ ÇÖKMESİ (o zamanki kısmi düzeltme) + NESNE ÖZEL TEMA

1. **ÇÖKME DÜZELTMESİ (editör):** Modern → Oldschool temaya dönüşte program kapanıyordu.
   **Kök neden:** eski sıra "bağlamı KAPAT → sonra comctl6 temalı panelleri yok et" idi;
   görsel stil kaynakları serbest bırakılırken erişim ihlali oluşuyordu (Old→New yönünde
   sorun yoktu, New→Old yönünde çöküyordu — tam kullanıcı raporuyla uyumlu).
   **Düzeltme:** sıra artık **yok et (eski bağlam aktifken) → bağlamı değiştir → yarat
   (yeni bağlamda)**. `DoThemeSwitch` üç aşamaya bölündü (`DestroyAllPanels` /
   `SetModern` / `CreateAllPanels`); tüm statik panel işaretçileri null'a çekilir,
   yüzey `WM_DESTROY`'u `IsWindow` korumalıdır. Üretilen kodda `M2ThemeSwitch` aynı
   düzeltilmiş sırayı kullanır ve `WM_DESTROY` `M2God::Form::Handle = NULL` yazar.

2. **NESNE ÖZEL TEMA:** kullanıcının istediği gibi bir nesnenin teması bağımsız
   değiştirilebilir:
   - **Özellikler panelinde `Theme` satırı** (her görsel kontrolde; TabPage hariç):
     combobox — `(Proje Varsayılanı)` / `Oldschool` / `Modern`. Katalog
     `enums["M2GodTheme"]` ile gelir (Config_info.json'da olsa da olmasa da garanti).
   - **Sağ tık bağlam menüsünde "Tema" alt menüsü** — aynı üç seçenek, mevcut
     seçim işaretli.
   - **Yüzeysel gerçekleme:** kontrolün etkin teması (özelliği varsa o, yoksa
     arayüz/proje teması) ile mevcut aktivasyon bağlamı farklıysa `theme::Push/Pop`
     tek kontrolü kendi bağlamı içinde yaratır — klasik ve temalı kontroller aynı
     formda BİRLİKTE yaşayabilir.
   - **Üretilen kodda:** `Theme` özelliği açık seçilmiş kontrollerin yaratım satırı
     `M2God::Theme::PushOverride(...)` / `M2God::Theme::PopOverride()` ile sarılır
     (Global.h v2901). Çalışma anında kullanıcı menüden tema değiştirse bile
     tasarımcının nesneye biçtiği tema korunur; kalıtsal kontroller menüyü izler.
   - `Theme` özelliği .m2god'a kaydedilir (Props sözlüğü birebir serileştirilir).

3. **Proje teması senkronu:** `Theme: Modern` kayıtlı proje açılınca editör arayüzü
   de otomatik Modern'e geçer (yüzey kontrolleri doğru temada kurulur).

**Doğrulama:** yeni `global_template.inc` + birebir emisyon blokları mingw ile
derlendi (`tests/themetest.cpp`, `work/gentest/themecheck.cpp`); eski üretilmiş proje
yeni Global.h ile sorunsuz derlenir (geriye uyumlu).

---

## Faz 2.9 — TEMA SİSTEMİ (Oldschool / Modern) + Kod Gezgini tam yenilendi

Kullanıcı isteği: *"Üstteki Kod Gezgini bir işe yaramadı. Formlar görünsün, aşağı doğru
genişlet şeklinde açılsın; içindekiler Form1 içindeki objeler olsun. MyButton1'e tıklarsam
UI editörde o seçilecek, özellikler/olaylar kısmında MyButton1'e ait şeyleri görürüm.
MyButton1'i genişletirsem eventleri (MyButton1_Click) gözüksün. Ayrıca tema seçenekleri
ekleyelim — editör ve editörün ürettiği koddaki UI çok oldschool; klasik .NET 4.8 (C++ CLR
WinForm) benzeri daha modern bir görünüm ikinci seçenek olsun ve UI'de de görebilelim."*

1. **TEMA MENÜSÜ (editör):** `theme.h/cpp` — menü çubuğunda yeni **"Tema"** menüsü:
   - **Oldschool (Klasik Win32)** — VARSAYILAN: mevcut görünüm (comctl32 v5, 3D kenarlar).
     Geriye dönük davranış birebir korunur.
   - **Modern (.NET 4.8 / WinForm benzeri)** — comctl32 v6 **görsel stiller**:
     XP/Win10 tarzı gradient düğmeler, ince kenarlıklı TextBox/ListView, temalı
     TreeView/TabControl/ProgressBar — klasik C++ CLR WinForm uygulaması hissi.
   - Gerçekleme: comctl6 manifest'i exe'ye **RT_MANIFEST id 2** olarak gömülür (id 1
     OLMAMALI — loader başlangıçta otomatik etkinleştirir ve varsayılan bozulur);
     `CreateActCtxW` + `ActivateActCtx/DeactivateActCtx` ile bağlam açılıp kapanır ve
     TÜM paneller + tasarım yüzeyi kontrolleri yeniden yaratılır (temalı çizim yalnız
     denetim yaratılırken seçilir). Menü işaretleri radio benzeri güncellenir.
   - Seçilen tema **.m2god proje dosyasına** kaydedilir (`"Theme": "Modern"/"Oldschool"`)
     ve üretilen kodun başlangıç teması olur.

2. **ÜRETİLEN UYGULAMADA "Tema" MENÜSÜ:** her üretilen form, menü barında (tasarımda
   MenuStrip varsa SONUNA, yoksa tek başına) **"Tema"** menüsü taşır:
   `Modern (.NET 4.8 / WinForm benzeri)` + `Oldschool (Klasik Win32)` — çalışma anında
   geçiş: `M2God::Theme::Set()` bağlamı değiştirir, `M2ThemeSwitch` formu YENİDEN YARATIR
   (`Recreating` bayrağı WM_DESTROY'da `PostQuitMessage`'ı atlar — uygulama KAPANMAZ,
   Form1'in mesaj döngüsü sürer). Seçim `HKCU\Software\M2GodWin32\Theme` kayıt
   defterinde hatırlanır; sonraki açılışta kullanıcının son tercihi geçerlidir.
   İkincil formlar da (Show\<Ad\>) tema menüsü taşır ve `M2CreateForm` ortak yaratma
   fonksiyonu üzerinden açılır.

3. **Global.h SDK: `M2God::Theme`** (v2901): `Startup(designerModern)` / `Set(modern)` /
   `Activate()` / `AttachMenu(form)` / manifest XML'i (Common-Controls 6 bağımlılığı)
   %TEMP%'e yazılır — **.rc dosyası GEREKMEZ**, saf kod çalışır (VS2019 varsayılan
   ayarlarıyla derlenir; kayıt defteri için Advapi32 VS varsayılan lib'lerindendir).
   Şablon statikleri `ThemeT<Tag>` sınıf şablonu üyeleri → çok .cpp'li projede ODR
   güvenli (tek varlık), C++14 uyumlu.

4. **KOD GEZGİNİ TAM YENİDEN YAZILDI** (`codetree.cpp` + `forms::TreeObjectsOf/TreeFormEventsOf`):
   - **TÜM formlar** kök düğüm ve TÜMÜ genişletilebilir (kalın = aktif form). Parklı
     formların objeleri tasarım JSON'undan okunur — aktif formu DEĞİŞTİRMEDEN ağaçta
     görülür.
   - Form genişletilince **TÜM OBJELER** listelenir (MyButton1, label1, timer1… —
     bileşenler dahil; eski sürüm yalnız olayı olan kontrolleri gösteriyordu = "bir işe
     yaramadı" sorununun kökü).
   - **Objeye tek tık:** editörde seçilir + Özellikler/Olaylar paneli o objeye geçer;
     başka formun objesine tıklanırsa önce o forma geçilir.
   - **Obje genişletilince olayları** (`MyButton1_Click`) görünür; işleyiciye tıkla →
     Olaylar sekmesi; **çift tık → KODA GİT** (VS2019 bağlıysa gerçek VS koduna, değilse
     üretim görüntüleyicisi).
   - **Konteyner hiyerarşisi:** Panel/GroupBox/TabPage alt kontrolleri ağaçta içe
     gömülü gösterilir (ebeveyn önce çözülür — döngü koruması ile).
   - Form düzeyi olaylar (`Form1_Load`) objelerden sonra; genişletme durumları
     (form + obje) hatırlanır; seçili düğüme TEKRAR tıklayınca da editör/panel
     senkronu tetiklenir (TVN_SELCHANGED yalnız seçim DEĞİŞİNce geldiği için
     NM_CLICK ile tamamlanır).

5. **Kod üretimi ortak yaratma:** `M2CreateForm(hInst, nCmdShow, owner)` — sınıf kaydı +
   pencere + tüm form özellikleri (BackColor/MaximizeBox/FormBorderStyle/TopMost/
   WindowState/CenterToScreen) TEK fonksiyonda; `wWinMain`, `Show<Ad>()` ve tema geçişi
   aynı yolu kullanır → tema geçişinde özellikler kaybolmaz.

6. `surface::RecreateAll` artık `IsWindow` korumalı (tema geçişi yüzeyi komple
   yeniden kurar;HWND'ler zaten ölü olabilir).

7. Derleme: `build_mingw.sh` — `theme.cpp` eklendi; çıktı `M2GodWin32_Faz2.9.exe`.
   `app.rc` RT_MANIFEST id 2 (`m2god_theme.manifest`) gömer; `.vcxproj` güncel.

---

## Faz 2.8 — RAD Studio Düzeni (sürüklenebilir ayraçlar + Kod Gezgini) + form tasarım etkileşimi

Kullanıcı isteği: *"sağdaki paneli 2 ayrı panele böl, yukarı aşağı fare ile sürükleyerek
büyütüp küçültebileyim; solda da olsun — RAD Studio'daki gibi: sağ üstte formlar, sağ altta
özellikler/olaylar; sol üstte koddaki açılmış event'ler (button1 → button1_Click), sol altta
araç kutusu. Formu da kontroller gibi fare ile büyütüp küçültebileyim. FormBorderStyle=None
editörde görsel olarak da yansın. Ayrıca VS2019 C++ sln'li örnek proje: Form1'deki düğme
Form2'yi açsın, Form2'deki düğme 'Ben Form2' diye MessageBox göstersin."*

1. **Sürüklenebilir panel AYRAÇLARI (splitter):** `splitter.h/cpp` — 6px'lik ayraç penceresi
   (`M2GOD_SPLIT`). Sol sütun: üstte **Kod Gezgini**, altta **Araç Kutusu**; sağ sütun:
   üstte **Formlar**, altta **Özellikler/Olaylar**. Ayraç fareyle sürüklenir (SIZENS imleç),
   üst panel yüksekliği canlı değişir; **çift tık** varsayılan yüksekliğe döner. `DoLayout`
   yeni düzeni kurar; kıstırma: alt panele en az 200–240px kalır. Tek panel görünürken
   ayraç gizlenir; "Görünüm" menüsünden Kod Gezgini / Araç Kutusu ayrı ayrı kapatılabilir.

2. **Kod Gezgini (sol üst panel):** `codetree.h/cpp` — RAD Studio Structure View tarzı ağaç:
   her form kök düğüm (aktif form kalın) → olayı AÇILMIŞ kontroller → işleyici çocukları
   (`button1 → button1_Click`, isim değişmişse `manualbutton_Click`). Form düzeyi olayları
   (`Form1_Load`) form düğümünün altında. **Tek tık:** hedef forma geç + kontrolü seç +
   Özellikler panelini Olaylar sekmesine çevir. **Çift tık = KODA GİT:** VS2019 bağlıysa
   `vsint::EnsureHandlerAndGoto` ile gerçek VS2019 koduna atlar; değilse işleyicinin
   üretilecek bloğunu gösteren kod görüntüleyici açılır. Ağaç, seçim/model/form
   değişimlerinde otomatik tazelenir; genişletme durumu hatırlanır.

3. **FORMU fareyle boyutlandırma:** form seçiliyken (boş form alanına tıkla) istemci
   alanının kenarında **8 boyutlandırma tutamacı** belirir; sürükleyince `formW/formH`
   (= **ClientSize**) canlı değişir, özellik panelindeki ClientSize satırı anında güncellenir.
   Sol/üst kenar tutamaçları formu yüzeyde kaydırır (kontroller istemci-görelisi kalır — VS
   davranışı). ESC iptal eder (başlangıç boyutuna döner), Geri Al çalışır.

4. **FormBorderStyle TASARIMCIYA YANSIR:** `surface::TitleH()` — stil **None** ise sahte
   başlık çubuğu çizilmez ve geometri kayar (derlenmiş exe ile görsel uyum). Ayrıca
   **MaximizeBox/MinimizeBox=False** başlıktaki büyüt/küçült düğmelerini soluklaştırır.

5. **Bileşen tepsisi istemcinin ALTINA taşındı** (önceden client alanını yiyordu):
   `ClientAreaRect` artık tam `formW × formH` — **ClientSize tasarımcıda birebir görünür**.

6. **Kod üretimi düzeltmesi (çok formlu):** ikincil form WndProc'unda `PostQuitMessage` artık
   YOK — Form2 kapanınca uygulama KAPANMAZ, Form1'in mesaj döngüsü sürer (WinForms davranışı).

7. **Model::ToJson ClientSize kalıcılığı:** `formW/formH` her kayıt/undo anında ClientSize
   olarak yazılır — form boyutlandırmaları kayıt ve Geri Al ile tutarlı kalır.

8. **Örnek VS2019 çözümü (ayrı teslim: `MultiFormOrnek_VS2019.zip`):** `.sln + .vcxproj (v142,
   Win32, Unicode, C++14)` + `Form1.cpp/h + Form2.cpp/h + Global.h + .m2god`.
   Form1'deki düğme `ShowForm2(hWnd)` ile Form2'yi açar; Form2'deki düğme
   `MessageBox(L"Ben Form2")` gösterir. Kod üreticisinin çıktısıyla birebir aynı yapıdadır.

---

## Faz 2.7 — ÇOK FORMLU PROJE (özet)

- Sağ üstte FORMLAR listesi (+ Yeni Form), sağ altta Özellikler/Olaylar.
- Tek projede birden çok form (`forms.h/cpp` park/unpark; her formun kendi undo yığını).
- Kaydet/yükle: tüm formlar TEK `.m2god` (format `m2god-project` v3; eski dosyalar uyumlu).
- Kod üretimi (F6): her form kendi `<Ad>.h/.cpp` + Global.h; wWinMain ilk formda,
  diğerleri `Show<Ad>(owner)`; VS senkronu (F7) aktif forma çalışır.

---

## Derleme

- **Windows / VS2019:** `M2GodWin32.sln` → v142, Win32, Unicode.
- **Linux cross (MinGW):** `./build_mingw.sh` → `Bin/M2GodWin32_Faz2.8.exe`
  (x86_64-w64-mingw32-g++, `-municode -mwindows`, statik bağlantı).
- Çalıştırma: exe yanına `Config_info.json` koyun (yoksa yerleşik katalog kullanılır).

## Dosyalar

`src/splitter.cpp` (yeni), `src/codetree.cpp` (yeni) — diğer dosyalarda Faz 2.8
değişiklikleri: `mainwnd.cpp` (layout/paneller/gezinti/sürüm), `surface.cpp`
(form boyutlandırma + kenar stili görseli + geometri), `props.cpp` (ClientSize/FormBorderStyle
canlı yansıma + SetEventsMode), `model.cpp` (ClientSize kalıcılığı), `codegen.cpp`
(ikincil form PostQuitMessage düzeltmesi), `app.h/app.rc/build_mingw.sh` (2.8).
