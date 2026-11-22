// ============================================================================
//  canvas.cpp — WYSIWYG tasarım yüzeyi (TAM YENİDEN YAZIM).
//
//  Mimari:
//   1) "##canvas_host" penceresi canvas alanını kaplar (zemin + ızgara).
//   2) Tasarlanan form GERÇEK ImGui penceresi — ÜRETİM temasıyla çizilir
//      (theme::PushProduction): editörde görünen = derlenince göreceğin.
//   3) Her nesne objstyle::Push ile kendi stil/renk geçersiz kılmalarıyla
//      çizilir (üretilen .cpp ile AYNI push dizisi).
//   4) Tüm etiketler "metin##ad" → ImGui ID çakışması kesin olarak yok.
//   5) Hit-dikdörtgenleri GetItemRectMin/Max ile toplanır → tıklama, seçim
//      ve tutamaçlar GERÇEK çizim konumuyla hizalı (eski kayma hatası gitti).
//   6) Çoklu seçim: tık / Ctrl+tık / rubber band (boş alanda sürükle) /
//      grup taşıma / ok tuşları / Delete / Ctrl+C,X,V.
//   7) Tutamaç üzerinde imleç: NWSE/NS/NESW/EW boyutlandırma imleçleri.
// ============================================================================
#include "canvas.h"
#include "ui_editor.h"
#include "forms.h"
#include "theme.h"
#include "objstyle.h"
#include "codegen.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include "imgui_internal.h"

namespace canvas {

std::string g_armTool;
Ctrl* g_armParent = nullptr;

// 6. TUR: şu an çizilen nesnenin klip yüzeyi (DrawCtrl özyinelemesi)
static ImVec2 s_curClipMin, s_curClipMax;
// sürükleme sonunda reparent edilecek kökler (drop hedefiyle birlikte)
static std::vector<Ctrl*> s_dragRoots;

// ---------------------------------------------------------------------------
//  durum
static ImVec2 s_canvasMin, s_canvasMax;
static bool   s_hostHovered = false;      // canvas alanı hover (form dahil değil)

// rubber band (toplu seçim)
static bool   s_band = false;
static ImVec2 s_bandStart, s_bandCur;

// taşıma / boyutlandırma
static bool s_dragBody = false;            // nesne gövdesi sürükleme (grup)
static int   s_dragHandle = -1;            // birincil seçim tutamacı 0..7
static ImVec2 s_dragStart;
static std::map<Ctrl*, std::pair<int,int>> s_dragOrig;   // taşıma başlangıç konumları
static int s_origX, s_origY, s_origW, s_origH;            // boyutlandırma başlangıcı
static bool s_formDrag = false;            // form tutamacı sürükleme
static int  s_formHandle = -1;

// bu karede taranabilir yüzeyler (GERÇEK çizim konumları)
// 6. TUR: cclip — çizildiği andaki KLİP dikdörtgeni (konteyner içi çocuklar
// konteyner yüzeyine kırpılır). Tamamen kırpılan çocuk TIKLANAMAZ (WinForms
// davranışı: panel dışına taşan görünmez çocuk seçilmez).
struct WidgetRect { Ctrl* c; ImVec2 rmin, rmax; bool tray = false; ImVec2 cclipMin, cclipMax; float topPad = 0; };
static std::vector<WidgetRect> s_hitRects;
static ImVec2 s_formPos, s_formSize;        // çizilen formun gerçek dikdörtgeni

// kalıcı metin arabellekleri (her InputText nesnesinin kendi tamponu)
static std::map<const Ctrl*, std::string> s_textBufs;
static std::map<const Ctrl*, std::string> s_lastVal;    // Value senkron izi (dosya kapsamında — silinince temizlenir)

// aktif sekme başlığı (TabBar yerleştirmesinde TabOwner için)
static std::map<const Ctrl*, std::string> s_tabActiveOwner;

// sağ tık bağlam menüsü durumu (nesne / form menüleri)
static bool   s_ctxRequest = false;     // bu karede menü açma isteği
static Ctrl*  s_ctxTarget = nullptr;   // menü hedefi (null → form/canvas menüsü)

void ArmTool(const std::string& tool) { g_armTool = tool; g_armParent = nullptr; }
bool IsArmed() { return !g_armTool.empty(); }
void CancelArm() { g_armTool.clear(); g_armParent = nullptr; }

void Select(Ctrl* c, bool additive) {
    if (additive) g_model.ToggleSel(c);
    else g_model.SelectOnly(c);
}
void SelectForm() { g_model.SelectForm(); }

ImVec2 CtrlAbsPos(const Ctrl* c) {
    int x = c->x, y = c->y;
    for (Ctrl* p = c->parent; p; p = p->parent) { x += p->x; y += p->y; }
    return ImVec2((float)x, (float)y);
}

// ---------------------------------------------------------------------------
//  form başlık bayrakları (üretilen kodla aynı set + editör sabitlemeleri)
static ImGuiWindowFlags FormWinFlagsOf(const Model& m) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    auto it = m.formProps.find("Flags");
    if (it != m.formProps.end())
        for (auto& s : SplitW(Utf8ToWideStr(it->second), L'|')) {
            std::string t = TrimA(WideStrToUtf8(s));
            if (t == "NoTitleBar") flags |= ImGuiWindowFlags_NoTitleBar;
            else if (t == "NoResize") flags |= ImGuiWindowFlags_NoResize;
            else if (t == "NoMove") flags |= ImGuiWindowFlags_NoMove;
            else if (t == "AlwaysAutoResize") flags |= ImGuiWindowFlags_AlwaysAutoResize;
            else if (t == "NoScrollbar") flags |= ImGuiWindowFlags_NoScrollbar;
            else if (t == "MenuBar") flags |= ImGuiWindowFlags_MenuBar;
        }
    for (Ctrl* c : m.ctrls) if (c->type == "Menu") { flags |= ImGuiWindowFlags_MenuBar; break; }
    // editör sabitlemeleri (tasarım yüzeyinde form sabit kalır)
    flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    return flags;
}

// ---------------------------------------------------------------------------
//  PushRect: gerçek çizim dikdörtgenini kaydet + WYSIWYG BOYUT SENKRONU
static void SyncSizeProps(Ctrl* c, int w, int h) {
    // "Width/Height/SizeArgW/SizeArgH" özellikleri tasarım boyutuyla aynı sayıyı
    // göstersin (özellik paneli ↔ canvas ↔ kod üretimi tek gerçek).
    auto sw = c->props.find("Width");   if (sw != c->props.end()) sw->second = std::to_string(w);
    auto sh = c->props.find("Height"); if (sh != c->props.end()) sh->second = std::to_string(h);
    auto swz = c->props.find("SizeArgW"); if (swz != c->props.end()) swz->second = std::to_string(w);
    auto shz = c->props.find("SizeArgH"); if (shz != c->props.end()) shz->second = std::to_string(h);
}
static void PushRect(Ctrl* c, bool tray = false) {
    WidgetRect r; r.c = c;
    r.rmin = ImGui::GetItemRectMin();
    r.rmax = ImGui::GetItemRectMax();
    r.tray = tray;
    r.cclipMin = s_curClipMin;
    r.cclipMax = s_curClipMax;
    s_hitRects.push_back(r);
    if (tray) return;
    // HATA DÜZELTİLDİ (kullanıcı: "boyut küçülüp büyüse de görselde değişmiyor,
    // ama özelliklerdeki Boyut değişiyor"): AUTO tipler her kare GERÇEK çizilen
    // boyutla senkron; yalnız-genişlik tiplerde yükseklik gerçek çizimden gelir.
    // Böylece özellik panelindeki Boyut daima görüntüyle AYNI — hayalet boyut yok.
    int cap = SizeCapOf(c);
    if (cap == SIZECAP_AUTO) {
        int rw = (int)(r.rmax.x - r.rmin.x + 0.5f), rh = (int)(r.rmax.y - r.rmin.y + 0.5f);
        if (rw > 0 && rw != c->w) { c->w = rw; SyncSizeProps(c, rw, rh); }
        else if (rh > 0 && rh != c->h) { c->h = rh; SyncSizeProps(c, c->w, rh); }
    } else if (cap == SIZECAP_W) {
        int rh = (int)(r.rmax.y - r.rmin.y + 0.5f);
        if (rh > 0 && rh != c->h) { c->h = rh; SyncSizeProps(c, c->w, rh); }  // yükseklik ImGui sabiti
    } else if (cap == SIZECAP_FULL && c->h == 0 &&
               (IsFrameSizedType(c->type) || c->type == "Checkbox" || c->type == "RadioButton")) {
        // 6. TUR: çerçeve ailesi + Checkbox/Radio'da h henüz elle değnilmemişse (0)
        // gerçek çizim yüksekliğiyle senkronlanır — panel "0" göstermez, sürüklemede kalır.
        int rh = (int)(r.rmax.y - r.rmin.y + 0.5f);
        if (rh > 0 && rh != c->h) { c->h = rh; SyncSizeProps(c, c->w, rh); }
    }
}
// 6. TUR: konteyner hit dikdörtgesi — child pencere KENDİ dikdörtgesinden kayıt
// (ÇOCUKLARDAN ÖNCE!). Böylece z-sırası gerçek olur: çocuklar konteynerden
// SONRA çizilir → çocuk kazanır; form-seviyesi sonraki nesneler daha da sonra
// çizilir → onlar kazanır (WinForms görsel z-sırası).
static void PushRectXY(Ctrl* c, ImVec2 rmin, ImVec2 rmax) {
    WidgetRect r; r.c = c;
    r.rmin = rmin; r.rmax = rmax;
    r.tray = false;
    r.cclipMin = s_curClipMin; r.cclipMax = s_curClipMax;
    s_hitRects.push_back(r);
}
// hit dikdörtgenini en az w genişliğe büyüt (tutamaçla genişletilen "alan"
// tipleri: Checkbox/RadioButton/BulletText — WinForms'taki gibi kontrol
// alanı w'dir; görsel içerik sol tarafta kalır)
static void PushRectMinWidth(Ctrl* c, int minW) {
    if (s_hitRects.empty() || s_hitRects.back().c != c) return;
    ImVec2& rmax = s_hitRects.back().rmax;
    float want = s_hitRects.back().rmin.x + (float)(minW > 0 ? minW : 0);
    if (rmax.x < want) rmax.x = want;
}
// 6. TUR: son hit dikdörtgeninin üst kenarını uzatır (GroupBox başlığı tıklanır
// olsun — başlık çerçevenin ÜSTÜNDE çizilir, hit alanının dışında kalırdı).
// topPad kaydedilir: reparent koordinat çevrimi GERÇEK çerçeveye göre yapılır.
static void PushRectTopPad(Ctrl* c, float pad) {
    if (s_hitRects.empty() || s_hitRects.back().c != c) return;
    s_hitRects.back().rmin.y -= pad;
    s_hitRects.back().topPad = pad;
}
// güvenli: widget çizilmediyse (0 boyut vb.) sağlam bir dikdörtgen kaydet
static void PushRectFallback(Ctrl* c) {
    WidgetRect r; r.c = c;
    ImVec2 p = ImGui::GetCursorScreenPos();
    r.rmin = p;
    r.rmax = ImVec2(p.x + (float)(c->w ? c->w : 60), p.y + (float)(c->h ? c->h : 20));
    r.cclipMin = s_curClipMin;
    r.cclipMax = s_curClipMax;
    s_hitRects.push_back(r);
}

// nesne etiketi: görünen metin + benzersiz kimlik ("Buton##button1")
// 6. TUR (kullanıcı: "textboxu oluşturuyom yanında string ile 'Yazı'...");
// g_hideLabels açıkken YAN-METİNLİ tiplerin görünen kısmı yok olur — yalnız
// "##ad" kimliği kalır (ID benzersizliği korunur; üretilen kod aynı şeyi yazar).
static std::string WLabel(const Ctrl* c, const char* textProp, const char* defText) {
    std::string t = c->Get(textProp, defText);
    if (g_hideLabels && HasAdjacentLabel(c->type)) t.clear();
    return t + "##" + c->name;
}

// 6. TUR: tek-satır çerçeve ailesi için yükseklik dönüşümü — FramePadding.y
// ölçeklenir → çerçeve satırı c->h olur. Checkbox/Radio zaten aynı dönüşümü
// kullanıyor; IGOD sarmalayıcıları (üretim) BİREBİR aynısını yazar (WYSIWYG).
static bool PushFrameHeight(int h) {
    if (h <= 0) return false;
    float fh = ImGui::GetFontSize();
    float padY = (h - fh) * 0.5f;
    if (padY < 0.0f) return false;
    ImVec2 fp = ImGui::GetStyle().FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fp.x, padY));
    return true;
}

// etiketi avail genişliğine sığdır — taşarsa "…" ile kısalt (UTF-8 güvenli).
// Üretilen IGOD:: yardımcıları AYNI algoritmayı kullanır (WYSIWYG).
static std::string FitLabelW(const std::string& text, float avail) {
    if (avail <= 8.0f) return text;
    if (ImGui::CalcTextSize(text.c_str(), nullptr, true).x <= avail) return text;
    const std::string ell = "...";
    float budget = avail - ImGui::CalcTextSize(ell.c_str()).x;
    size_t i = 0;
    while (i < text.size()) {
        size_t len = 1;
        unsigned char ch = (unsigned char)text[i];
        if (ch >= 0xF0) len = 4; else if (ch >= 0xE0) len = 3; else if (ch >= 0xC0) len = 2;
        if (ImGui::CalcTextSize(text.c_str(), text.c_str() + i + len, true).x > budget) break;
        i += len;
    }
    std::string out = text.substr(0, i);
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out + ell;
}

// ---------------------------------------------------------------------------
//  nesne çizimi — üretilen kodla BİREBİR aynı ImGui dizisi (stil push'ları dahil)
//  clipMin/clipMax: bu nesnenin GÖRÜNÜR yüzeyi (konteyner içi çocuklar için
//  konteynerin klip dikdörtgeni — hit-test klip farkındalı çalışır).
static void DrawCtrl(Ctrl* c, Model& m, ImVec2 clipMin, ImVec2 clipMax) {
    s_curClipMin = clipMin;   // PushRect bu klip yüzeyini kaydeder (hit-test)
    s_curClipMax = clipMax;
    const std::string& t = c->type;
    const ImVec2 pos((float)c->x, (float)c->y);
    int stylePacked = objstyle::Push(c);
    bool disabled = c->GetB("Disabled");
    if (disabled) ImGui::BeginDisabled();

    if (t == "Button") {
        int w = atoi(c->Get("SizeArgW", "120").c_str());
        int h = atoi(c->Get("SizeArgH", "28").c_str());
        ImVec2 sz((float)(c->w > 0 ? c->w : w), (float)(c->h > 0 ? c->h : h));
        ImGui::SetCursorPos(pos);
        if (c->GetB("Small")) ImGui::SmallButton(WLabel(c, "Text", "Buton").c_str());
        else ImGui::Button(WLabel(c, "Text", "Buton").c_str(), sz);
        PushRect(c);
    } else if (t == "Checkbox" || t == "RadioButton") {
        // BOYUT UYGULA (W+H — kullanıcı: "büyütüp küçültemiyorum"): yükseklik
        // FramePadding ölçeklemesiyle KAREYİ ve satırı büyütür; genişlik etiketi
        // sığdırır (kısaltır) ve kontrol alanını genişletir. Üretilen IGOD
        // sarmalayıcıları AYNI dönüşümü uygular (WYSIWYG).
        bool isCheck = (t == "Checkbox");
        bool v = isCheck ? c->GetB("Checked") : c->GetB("Active");
        std::string lbl = WLabel(c, "Text", isCheck ? "Seçenek" : "Seçim");
        size_t hashPos = lbl.find("##");
        std::string visible = lbl.substr(0, hashPos);
        std::string idSfx = lbl.substr(hashPos);
        bool padPush = false;
        if (c->h > 0) {
            float fh = ImGui::GetFontSize();
            float padY = (c->h - fh) * 0.5f;
            if (padY >= 0.0f) {
                ImVec2 fp = ImGui::GetStyle().FramePadding;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fp.x, padY));
                padPush = true;
            }
        }
        float square = ImGui::GetFrameHeight();
        std::string shown = visible;
        if (c->w > 0)
            shown = FitLabelW(visible, (float)c->w - square - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::SetCursorPos(pos);
        if (isCheck) ImGui::Checkbox((shown + idSfx).c_str(), &v);
        else         ImGui::RadioButton((shown + idSfx).c_str(), &v);
        if (padPush) ImGui::PopStyleVar();
        PushRect(c);
        PushRectMinWidth(c, c->w);
    } else if (t == "SliderFloat" || t == "SliderInt") {
        float fv = (float)atof(c->Get("Value", "0").c_str());
        int   iv = atoi(c->Get("Value", "0").c_str());
        float mn = (float)atof(c->Get("Min", "0").c_str());
        float mx = (float)atof(c->Get("Max", "100").c_str());
        const char* fmt = c->Get("Format", t == "SliderFloat" ? "%.1f" : "%d").c_str();
        // 6. TUR: yükseklik FramePadding ölçeklemesi (canvas ↔ IGOD aynı — WYSIWYG)
        bool padPush = PushFrameHeight(c->h);
        ImGui::SetCursorPos(pos);
        if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);   // BOYUT UYGULA (W)
        if (t == "SliderFloat") ImGui::SliderFloat(WLabel(c, "Text", "Değer").c_str(), &fv, mn, mx, fmt);
        else ImGui::SliderInt(WLabel(c, "Text", "Değer").c_str(), &iv, (int)mn, (int)mx, fmt);
        if (padPush) ImGui::PopStyleVar();
        PushRect(c);
    } else if (t == "DragFloat" || t == "DragInt") {
        float fv = (float)atof(c->Get("Value", "0").c_str());
        int   iv = atoi(c->Get("Value", "0").c_str());
        float mn = (float)atof(c->Get("Min", "0").c_str());
        float mx = (float)atof(c->Get("Max", "100").c_str());
        bool padPush = PushFrameHeight(c->h);   // 6. TUR: dikey boyut
        ImGui::SetCursorPos(pos);
        if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);   // BOYUT UYGULA (W)
        if (t == "DragFloat") ImGui::DragFloat(WLabel(c, "Text", "Değer").c_str(), &fv, (float)atof(c->Get("Speed", "1").c_str()), mn, mx);
        else ImGui::DragInt(WLabel(c, "Text", "Değer").c_str(), &iv, (float)atof(c->Get("Speed", "1").c_str()), (int)mn, (int)mx);
        if (padPush) ImGui::PopStyleVar();
        PushRect(c);
    } else if (t == "InputText" || t == "InputTextMultiline" || t == "TextBox" || t == "TextBoxMultiline") {
        // her nesnenin KALICI kendi tamponu (Value'dan başlatılır)
        std::string& buf = s_textBufs[c];
        std::string curVal = c->Get("Value", "");
        // tasarımdaki Value değişti mi? (props panelinden) — senkron
        if (s_lastVal[c] != curVal) { buf = curVal; s_lastVal[c] = curVal; }
        // HATA DÜZELTİLDİ (bellek şişmesi): eski kod buf >= 512 iken HER KARE +1
        // byte ekliyordu (sınırsız büyüme). Artık yalnızca gerekliyse büyür.
        size_t need = curVal.size() + 1;
        if (need < 512) need = 512;
        if (buf.size() < need) buf.resize(need, '\0');
        ImGuiInputTextFlags fl = ImGuiInputTextFlags_ReadOnly;
        if (c->GetB("Password")) fl |= ImGuiInputTextFlags_Password;
        // 6. TUR (kullanıcı: "sağa sola büyütüp küçültebiliyorum da yukarı aşağı
        // yapamıyorum"): tek-satır TextBox/InputText yüksekliği FramePadding
        // ölçeklemesiyle büyütülür — canvas ↔ IGOD::InputText aynı dönüşüm.
        bool padPush = (t == "InputText" || t == "TextBox") && PushFrameHeight(c->h);
        ImGui::SetCursorPos(pos);
        if (t == "InputText" || t == "TextBox") {
            if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);   // BOYUT UYGULA (W)
            ImGui::InputText(WLabel(c, "Text", "Yazı").c_str(), &buf[0], (int)buf.size(), fl);
        }
        else {
            ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "280").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "120").c_str())));
            ImGui::InputTextMultiline(WLabel(c, "Text", "Not").c_str(), &buf[0], buf.size(), sz, fl);
        }
        if (padPush) ImGui::PopStyleVar();
        PushRect(c);
    } else if (t == "InputInt" || t == "InputFloat") {
        int iv = atoi(c->Get("Value", "0").c_str());
        float fv = (float)atof(c->Get("Value", "0").c_str());
        bool padPush = PushFrameHeight(c->h);   // 6. TUR: dikey boyut
        ImGui::SetCursorPos(pos);
        if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);   // BOYUT UYGULA (W)
        if (t == "InputInt") ImGui::InputInt(WLabel(c, "Text", "Sayı").c_str(), &iv, atoi(c->Get("Step", "1").c_str()));
        else ImGui::InputFloat(WLabel(c, "Text", "Sayı").c_str(), &fv, (float)atof(c->Get("Step", "1").c_str()));
        if (padPush) ImGui::PopStyleVar();
        PushRect(c);
    } else if (t == "Combo" || t == "ListBox") {
        int sel = atoi(c->Get("Selected", "0").c_str());
        auto items = SplitW(Utf8ToWideStr(c->Get("Items", "")), L'|');
        static std::vector<std::string> s_store;      // kare içi ömür güvenli depo
        s_store.clear();
        for (auto& i : items) s_store.push_back(WideStrToUtf8(i));
        std::vector<const char*> ptrs;
        for (auto& s : s_store) ptrs.push_back(s.c_str());
        const char* cur = (sel >= 0 && sel < (int)ptrs.size()) ? ptrs[sel] : "";
        ImGui::SetCursorPos(pos);
        if (t == "Combo") {
            // 6. TUR: yükseklik FramePadding ölçeklemesi (canvas ↔ IGOD aynı)
            bool padPush = PushFrameHeight(c->h);
            if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);   // BOYUT UYGULA (W)
            if (ImGui::BeginCombo(WLabel(c, "Text", "Seçim").c_str(), cur)) ImGui::EndCombo();
            if (padPush) ImGui::PopStyleVar();
            PushRect(c);
        } else {
            // LISTBOX — TAM BOYUT (W+H): genişlik c->w, yükseklik c->h; h boşken
            // HeightItems satır sayısından (uyumluluk). Sürükle-büyüt artık GÖRÜNÜR.
            int hi = atoi(c->Get("HeightItems", "5").c_str());
            float hgt = c->h > 0 ? (float)c->h
                       : ImGui::GetTextLineHeightWithSpacing() * (hi + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
            float wid = c->w > 0 ? (float)c->w : 0.0f;
            if (ImGui::BeginListBox(WLabel(c, "Text", "Liste").c_str(), ImVec2(wid, hgt))) {
                for (auto& p : ptrs) ImGui::Selectable((std::string(p) + "##" + c->name).c_str(), false);
                ImGui::EndListBox();
            }
            PushRect(c);
        }
    } else if (t == "ColorEdit4") {
        ImVec4 col(atof(c->Get("R", "255").c_str()) / 255.0f,
                    atof(c->Get("G", "255").c_str()) / 255.0f,
                    atof(c->Get("B", "255").c_str()) / 255.0f,
                    atof(c->Get("A", "255").c_str()) / 255.0f);
        ImGui::SetCursorPos(pos);
        // genişlik YALNIZ girişli kipte görsel etki yapar (NoInputs kare = içerik ölçer)
        if (c->w > 0 && c->GetB("AlphaPreview")) ImGui::SetNextItemWidth((float)c->w);
        ImGui::ColorEdit4(WLabel(c, "Text", "Renk").c_str(), &col.x,
                          (c->GetB("AlphaPreview") ? 0 : ImGuiColorEditFlags_NoInputs));
        PushRect(c);
    } else if (t == "ProgressBar") {
        ImGui::SetCursorPos(pos);
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "200").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "18").c_str())));
        const char* ov = c->Get("Overlay", "").c_str();
        ImGui::ProgressBar((float)atof(c->Get("Value", "0.5").c_str()), sz, ov[0] ? ov : nullptr);
        PushRect(c);
    } else if (t == "Separator") {
        // BOYUT UYGULA (W+H — kullanıcı: "seperator boyutu ince çubuk, büyütüp
        // küçültemiyorum"): gerçek boyutlandırılabilir ayraç = W×H dolgu çubuğu,
        // ImGuiCol_Separator rengiyle (SeparatorColor özelliği objstyle'den
        // push edilmiş durumda). Üretilen IGOD::Separator AYNISINI çizer.
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "200").c_str())),
                  (float)(c->h ? c->h : atoi(c->Get("Height", "2").c_str())));
        ImGui::SetCursorPos(pos);
        ImGui::Dummy(sz);
        ImDrawList* dlSep = ImGui::GetWindowDrawList();
        dlSep->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                             ImGui::GetColorU32(ImGuiCol_Separator));
        PushRect(c);
    } else if (t == "SeparatorText") {
        // BOYUT UYGULA (W): ImGui::SeparatorText mevcut pencere genişliğine
        // yayılır → w'lik sıfır-dolgu child'ına alınır: çizgi w'ye kadar çizilir
        // (üretim IGOD::SeparatorText AYNISINI yapar).
        ImGui::SetCursorPos(pos);
        if (c->w > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild((c->name + "_stw").c_str(), ImVec2((float)c->w, 0.0f), ImGuiChildFlags_AutoResizeY);
            ImGui::SeparatorText(WLabel(c, "Text", "Bölüm").c_str());
            ImGui::EndChild();
            ImGui::PopStyleVar();
        } else {
            ImGui::SeparatorText(WLabel(c, "Text", "Bölüm").c_str());
        }
        PushRect(c);
    } else if (t == "Text" || t == "TextColored" || t == "BulletText" || t == "LabelText") {
        std::string lbl = WLabel(c, "Text", "Etiket");
        size_t hashPos = lbl.find("##");
        std::string visible = lbl.substr(0, hashPos);
        std::string idSfx = lbl.substr(hashPos);
        ImGui::SetCursorPos(pos);
        if (t == "LabelText") {
            // BOYUT UYGULA (W — kullanıcı: "labeltext'in boyutunu kısaltamıyorum"):
            // LabelText değeri öğe genişliğinin SAĞ ucunda hizalar (CalcItemWidth);
            // SetNextItemWidth ile tasarım genişliği birebir geçer. Üretilen kod
            // aynı çağrıyı yazar (WYSIWYG).
            if (c->w > 0) ImGui::SetNextItemWidth((float)c->w);
            ImGui::LabelText(lbl.c_str(), "%s", c->Get("Value", "değer").c_str());
            PushRect(c);
        } else {
            // BOYUT UYGULA (W): metin w'de kayar. DİKKAT: PushTextWrapPos değeri
            // PENCERE-YEREL (1.91: CalcWrapWidthForPos → wrap_pos + WinPos - Scroll):
            // öğe imleç x'i + w. Üretilen kod aynı sabiti yazar (WYSIWYG).
            bool wrap = (c->w > 0) && t != "BulletText";
            if (wrap) ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + (float)c->w);
            if (t == "Text") {
                if (c->GetB("Wrapped") && !wrap) ImGui::TextWrapped("%s", visible.c_str());
                else ImGui::Text("%s", visible.c_str());
            }
            else if (t == "TextColored") {
                ImVec4 col(atof(c->Get("R", "255").c_str()) / 255.0f,
                            atof(c->Get("G", "200").c_str()) / 255.0f,
                            atof(c->Get("B", "80").c_str()) / 255.0f, 1.0f);
                ImGui::TextColored(col, "%s", visible.c_str());
            }
            else {
                std::string shown = visible;
                if (c->w > 0)
                    shown = FitLabelW(visible, (float)c->w - (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.x * 2.0f));
                ImGui::BulletText("%s", shown.c_str());
            }
            if (wrap) ImGui::PopTextWrapPos();
            PushRect(c);
            if (t == "BulletText") PushRectMinWidth(c, c->w);
        }
    } else if (t == "PlotLines" || t == "PlotHistogram") {
        ImGui::SetCursorPos(pos);
        std::vector<float> data;
        for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "")), L'|')) data.push_back((float)wcstod(s.c_str(), nullptr));
        if (data.empty()) data.push_back(0.0f);
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "280").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "120").c_str())));
        std::string lbl = WLabel(c, "Text", "Grafik");
        if (t == "PlotLines")
            ImGui::PlotLines(lbl.c_str(), data.data(), (int)data.size(), 0, nullptr,
                             (float)atof(c->Get("ScaleMin", "0").c_str()), (float)atof(c->Get("ScaleMax", "1").c_str()), sz);
        else
            ImGui::PlotHistogram(lbl.c_str(), data.data(), (int)data.size(), 0, nullptr, FLT_MAX, FLT_MAX, sz);
        PushRect(c);
    } else if (t == "Panel") {
        // PANEL — WinForms Panel karşılığı. ImGui SDK'da "panel" = kenarlıklı child
        // bölge (BeginChild): nesneleri İÇİNE yerleştirilebilen taşıyıcı yüzey.
        // (Kullanıcı talebi: araç kutusunda Panel — ImGui evrenindeki karşılığı budur.)
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "300").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "200").c_str())));
        ImGui::SetCursorPos(pos);
        ImGuiChildFlags cf = c->GetB("Border") ? ImGuiChildFlags_Borders : 0;
        if (ImGui::BeginChild(WLabel(c, "Text", "Panel").c_str(), sz, cf)) {
            // 6. TUR: konteyner hit dikdörtgesi ÇOCUKLARDAN ÖNCE kayıt (z-sırası);
            // çocuklar panel yüzeyine KIRPILIR — hit-test de aynı klip yüzeyini
            // kullanır (panel dışına taşan görünmez çocuk tıklanamaz).
            ImGuiWindow* cw = ImGui::GetCurrentWindow();
            PushRectXY(c, cw->Rect().Min, cw->Rect().Max);
            ImVec2 cm = cw->ClipRect.Min, cx2 = cw->ClipRect.Max;
            for (Ctrl* ch : m.ChildrenOf(c)) DrawCtrl(ch, m, cm, cx2);
        }
        ImGui::EndChild();
    } else if (t == "ChildWindow") {
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "300").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "200").c_str())));
        ImGui::SetCursorPos(pos);
        ImGuiChildFlags cf = c->GetB("Border") ? ImGuiChildFlags_Borders : 0;
        if (ImGui::BeginChild(c->name.c_str(), sz, cf)) {
            ImGuiWindow* cw = ImGui::GetCurrentWindow();
            PushRectXY(c, cw->Rect().Min, cw->Rect().Max);   // çocuklardan ÖNCE (z)
            ImVec2 cm = cw->ClipRect.Min, cx2 = cw->ClipRect.Max;
            for (Ctrl* ch : m.ChildrenOf(c)) DrawCtrl(ch, m, cm, cx2);
        }
        ImGui::EndChild();
    } else if (t == "GroupBox") {
        // üretilen IGOD::GroupBox ile aynı görünüm: çerçeveli child + üst başlık
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "320").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "160").c_str())));
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::BeginChild(c->name.c_str(), sz, ImGuiChildFlags_Borders);
        {
            ImGuiWindow* gw = ImGui::GetCurrentWindow();
            ImVec2 rmin = gw->Rect().Min, rmax = gw->Rect().Max;
            ImVec2 cm = gw->ClipRect.Min, cx2 = gw->ClipRect.Max;
            std::string txt = c->Get("Text", "Grup");
            ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
            // 6. TUR: hit dikdörtgesi ÇOCUKLARDAN ÖNCE (z-sırası) + başlık uzantısı
            // (başlık çerçevenin ÜSTÜNDE çizilir — başlığa tıklayınca seçilir).
            PushRectXY(c, rmin, rmax);
            PushRectTopPad(c, ts.y * 0.5f + 2.0f);
            for (Ctrl* ch : m.ChildrenOf(c)) DrawCtrl(ch, m, cm, cx2);
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(rmin, rmax, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
            ImVec2 lp = ImVec2(rmin.x + 8, rmin.y - ts.y * 0.5f);
            dl->AddRectFilled(ImVec2(lp.x - 4, lp.y - 1), ImVec2(lp.x + ts.x + 4, lp.y + ts.y + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));
            dl->AddText(lp, ImGui::GetColorU32(ImGuiCol_Text), txt.c_str());
        }
    } else if (t == "CollapsingHeader" || t == "TreeNode") {
        // BOYUT UYGULA (W): başlık mevcut genişliğe yayılır → w'lik sıfır-dolgu
        // child'ına alınır (üretim IGOD::CollapsingHeader/TreeNode aynı).
        ImGui::SetCursorPos(pos);
        ImGuiTreeNodeFlags fl = c->GetB("DefaultOpen") ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        bool wrapped = c->w > 0;
        if (wrapped) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild((c->name + "_hcw").c_str(), ImVec2((float)c->w, 0.0f), ImGuiChildFlags_AutoResizeY);
        }
        bool open = (t == "CollapsingHeader") ? ImGui::CollapsingHeader(WLabel(c, "Text", "Başlık").c_str(), fl)
                                              : ImGui::TreeNodeEx(WLabel(c, "Text", "Düğüm").c_str(), fl);
        PushRect(c);   // rect = başlığın kendisi (çocuklar çizilmeden ÖNCE al)
        if (open) {
            ImVec2 cm = ImGui::GetCurrentWindow()->ClipRect.Min, cx2 = ImGui::GetCurrentWindow()->ClipRect.Max;
            for (Ctrl* ch : m.ChildrenOf(c)) DrawCtrl(ch, m, cm, cx2);
            if (t == "TreeNode") ImGui::TreePop();
        }
        if (wrapped) { ImGui::EndChild(); ImGui::PopStyleVar(); }
    } else if (t == "TabBar") {
        // BOYUT UYGULA (W+H — kullanıcı: "tabbar boyutunu küçültüp
        // büyültemiyorum"): sekmeler W×H'lik sıfır-dolgu child bölgesine çizilir —
        // genişlik VE yükseklik tasarım boyutu. Üretilen IGOD::TabBar AYNI
        // sarmalamayı yapar (WYSIWYG).
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "400").c_str())),
                  (float)(c->h ? c->h : atoi(c->Get("Height", "220").c_str())));
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild((c->name + "_tbw").c_str(), sz, ImGuiChildFlags_None);
        {
            // 6. TUR: tabbar hit dikdörtgesi child pencereden, ÇOCUKLARDAN ÖNCE
            // (rect = tüm W×H bölge — konteyner davranışı).
            ImGuiWindow* cw = ImGui::GetCurrentWindow();
            PushRectXY(c, cw->Rect().Min, cw->Rect().Max);
        }
        if (ImGui::BeginTabBar(c->name.c_str(), ImGuiTabBarFlags_None)) {
            for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Genel|Ayarlar")), L'|')) {
                std::string title = WideStrToUtf8(s);
                if (ImGui::BeginTabItem(title.c_str())) {
                    s_tabActiveOwner[c] = title;   // aktif sekme (yerleştirme TabOwner için)
                    // 6. TUR: sekme içeriği tabbar bölgesine kırpılır (hit-test de)
                    ImVec2 cm = ImGui::GetCurrentWindow()->ClipRect.Min, cx2 = ImGui::GetCurrentWindow()->ClipRect.Max;
                    for (Ctrl* ch : m.ChildrenOf(c)) {
                        if (ch->Get("TabOwner", "") == title) DrawCtrl(ch, m, cm, cx2);
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    } else if (t == "Table") {
        ImGui::SetCursorPos(pos);
        int cols = atoi(c->Get("Columns", "3").c_str());
        int rows = atoi(c->Get("Rows", "3").c_str());
        std::vector<std::string> heads;
        for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Sütun1|Sütun2|Sütun3")), L'|'))
            heads.push_back(WideStrToUtf8(s));
        ImVec2 outer((float)(c->w > 0 ? c->w : 0), 0.0f);   // BOYUT UYGULA (W — outer_size)
        if (ImGui::BeginTable(c->name.c_str(), cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, outer)) {
            for (auto& h : heads) ImGui::TableSetupColumn(h.c_str());
            ImGui::TableHeadersRow();
            for (int r = 0; r < rows; r++) {
                ImGui::TableNextRow();
                for (int ci = 0; ci < cols; ci++) { ImGui::TableNextColumn(); ImGui::TextUnformatted("-"); }
            }
            ImGui::EndTable();
        }
        PushRect(c);
    } else if (t == "Menu") {
        // form penceresinin menü çubuğu — Form çiziminde işlenir (MenuBar bayrağı)
    } else if (t == "Popup" || t == "Timer" || t == "FontRef") {
        // görsel değil — bileşen tepsisinde
    }

    if (disabled) ImGui::EndDisabled();
    objstyle::Pop(stylePacked);
}

// ---------------------------------------------------------------------------
//  tıklama noktasındaki nesne (tray dahil değil)
//  HATA DÜZELTİLDİ (kullanıcı: "groupbox/panel içine obje koyamıyorum"): eski
//  HitUnder kayıt sırasına göre SON kaydedilen dikdörtgeni (konteyneri!)
//  öne alıyordu — konteyner kendi çocu­klarından SONRA kaydedildiği için
//  PANEL İÇİNDEKİ NESNEYİ TIKLAMAK paneli seçiyordu; çocuk seçilemez,
//  taşınamaz, boyutlanamaz oluyordu. Artık "EN DERİN" (parent zinciri en
//  uzun) nesne kazanır; eşit derinlikte geç çizilen (üstteki) kazanır.
static int DepthOf(const Ctrl* c) {
    int d = 0;
    for (Ctrl* p = c->parent; p; p = p->parent) d++;
    return d;
}
static bool PtInRect(ImVec2 pt, ImVec2 rmin, ImVec2 rmax) {
    return pt.x >= rmin.x && pt.x <= rmax.x && pt.y >= rmin.y && pt.y <= rmax.y;
}
// 6. TUR: klip-yönlü ETKİN dikdörtgen — nesnenin GÖRÜNÜR kısmı (konteyner kırpması).
// Tamamen kırpılmışsa false (tıklanamaz / seçilemez / band yakalamaz).
static bool EffectiveRect(const WidgetRect& r, ImVec2& emin, ImVec2& emax) {
    emin = ImVec2(ImMax(r.rmin.x, r.cclipMin.x), ImMax(r.rmin.y, r.cclipMin.y));
    emax = ImVec2(ImMin(r.rmax.x, r.cclipMax.x), ImMin(r.rmax.y, r.cclipMax.y));
    return emin.x < emax.x && emin.y < emax.y;
}
static WidgetRect* HitUnder(ImVec2 pt) {
    // 6. TUR: GERÇEK Z-SIRASI — geç çizilen (görsel olarak ÜSTTE olan) kazanır.
    // Eski "en derin kazanır" kuralı yanlıştı: konteyner ÇOCUĞU, üzerine
    // çizilmiş form-seviyesi nesneden tıklamayı ÇALIYORDU. Konteyner dikdörtgeleri
    // artık çocuklardan ÖNCE kayıtlanır → recency = görsel z-sırası.
    WidgetRect* best = nullptr;
    for (auto& r : s_hitRects) {
        if (r.tray) continue;
        // tıklama yalnız GÖRÜNÜR (klip-yönlü) yüzeyde yakalanır
        ImVec2 emin, emax;
        if (!EffectiveRect(r, emin, emax)) continue;
        if (!PtInRect(pt, emin, emax)) continue;
        best = &r;
    }
    return best;
}
void ClickSelectUnder(ImVec2 pt, bool additive) {
    WidgetRect* r = HitUnder(pt);
    if (r) Select(r->c, additive);
    else SelectForm();
}

// ---------------------------------------------------------------------------
//  çift tıkla araç ekleme (araç kutusu çift tık → aktif forma)
bool AddArmedToForm() {
    if (g_armTool.empty()) return false;
    static int s_cascade = 0;
    s_cascade = (s_cascade + 1) % 8;
    int x = 24 + s_cascade * 16, y = 24 + s_cascade * 16;
    // 6. TUR: çift tık ekleme ARTIK en üstteki konteyneri yok saymaz — form
    // seviyesine ekler (M2God davranışı: toolbox çift tıkı forma ekler).
    Ctrl* c = g_model.Add(g_armTool, x, y, nullptr);
    return c != nullptr;
}

// ---------------------------------------------------------------------------
//  6. TUR: SÜRÜKLE-BIRAK RE-PARENT (kullanıcı: "nesneleri groupboxun içine
//  koyamıyorum koyduğumda... nesneler onun içinde değilmiş").
//  WinForms/M2God tasarımcı davranışı: nesne bir konteynerin İÇİNE bırakılınca
//  onun ÇOCUĞU olur — konteyner taşınsa içindekiler de taşır, küçültülse
//  kırpılır (BeginChild klip). DIŞINA bırakılınca forma döner.
static void ReparentAfterDrag() {
    if (s_dragRoots.empty()) return;
    for (Ctrl* c : s_dragRoots) {
        // bu karenin gerçek çizim dikdörtgeni (hâlâ modelde mi? undo geçebilir)
        bool alive = false;
        for (Ctrl* x : g_model.ctrls) if (x == c) { alive = true; break; }
        if (!alive) continue;
        WidgetRect* myr = nullptr;
        for (auto& r : s_hitRects) if (r.c == c && !r.tray) { myr = &r; break; }
        if (!myr) continue;   // çizilmiyor (kapalı sekme vb.) — parent dokunma
        ImVec2 ctr((myr->rmin.x + myr->rmax.x) * 0.5f, (myr->rmin.y + myr->rmax.y) * 0.5f);
        // hedef: MERKEZİN altındaki GEÇ ÇİZİLEN (görsel üstte) drop-hedefi konteyner
        // (kendisi ve alt ağacı hariç — döngü koruması). Konteynerler artık kendi
        // çocuklarından ÖNCE kayıtlandığından içteki konteyner DAHA GEÇ kayıtlanır
        // → recency = görsel z-sırası (en üstteki en içteki konteyner kazanır).
        Ctrl* tgt = nullptr;  WidgetRect* tgtRect = nullptr;
        for (auto& r : s_hitRects) {
            Ctrl* cc = r.c;
            if (r.tray || !IsDropTargetContainer(cc->type) || cc->nonVisual) continue;
            if (cc == c || g_model.IsAncestorOf(c, cc)) continue;   // cc c'nin içinde → olamaz
            if (!PtInRect(ctr, r.rmin, r.rmax)) continue;
            tgt = cc; tgtRect = &r;   // geç kayıtlanan = görsel olarak üstte
        }
        if (tgt && tgtRect) {
            if (c->parent != tgt) {
                // koordinat çevrimi GERÇEK çerçeve kenarından (topPad = başlık uzatması)
                int px = (int)(ctr.x - tgtRect->rmin.x);
                int py = (int)(ctr.y - (tgtRect->rmin.y + tgtRect->topPad));
                if (g_snapEnabled) {
                    px = (px + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
                    py = (py + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
                }
                g_model.Reparent(c, tgt, px, py);
                // TabBar'a bırakılan nesne AKTİF sekmenin çocuğu olur
                if (tgt->type == "TabBar") {
                    auto it = s_tabActiveOwner.find(tgt);
                    if (it != s_tabActiveOwner.end()) c->Set("TabOwner", it->second);
                }
                ui_editor::StatusUpdate(c->name + " \342\210\222 " + tgt->name + " i\303\247ine ta\305\237\304\261nd\304\261 (ta\305\237\304\261y\304\261c\304\261yla hareket eder)");
            }
        } else if (c->parent) {
            // hiçbir konteynerin içinde değil → FORMA DÖN (koordinat çevrimi)
            ImVec2 abs = CtrlAbsPos(c);
            int px = (int)abs.x, py = (int)abs.y;
            if (g_snapEnabled) {
                px = (px + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
                py = (py + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
            }
            g_model.Reparent(c, nullptr, px, py);
            ui_editor::StatusUpdate(c->name + " \342\210\222 forma ta\305\237\304\261nd\304\261 (konteyner d\304\261\305\237\304\261na b\304\261rak\304\261ld\304\261)");
        }
    }
    s_dragRoots.clear();
}

// ---------------------------------------------------------------------------
//  tutamaç yardımcıları
// 0:SO 1:O 2:DO 3:D 4:DD 5:G 6:SD 7:S
static void HandleCorners(ImVec2 rmin, ImVec2 rmax, ImVec2 out[8]) {
    out[0] = rmin;
    out[1] = ImVec2((rmin.x + rmax.x) / 2, rmin.y);
    out[2] = ImVec2(rmax.x, rmin.y);
    out[3] = ImVec2(rmax.x, (rmin.y + rmax.y) / 2);
    out[4] = rmax;
    out[5] = ImVec2((rmin.x + rmax.x) / 2, rmax.y);
    out[6] = ImVec2(rmin.x, rmax.y);
    out[7] = ImVec2(rmin.x, (rmin.y + rmax.y) / 2);
}
static int HandleIndexAt(ImVec2 pt, ImVec2 rmin, ImVec2 rmax, float hs, unsigned mask) {
    ImVec2 c[8];
    HandleCorners(rmin, rmax, c);
    for (int i = 0; i < 8; i++)
        if ((mask & (1u << i)) && fabsf(pt.x - c[i].x) <= hs && fabsf(pt.y - c[i].y) <= hs) return i;
    return -1;
}
// form yalnızca sağ/alt kenarlarından BÜYÜR (sol-üst köşe sabit — M2God/VS gibi):
// 2:sağ-üst 3:sağ 4:sağ-alt 5:alt 6:sol-alt. Sol/üst tutamaçları formlarda yok.
static int FormHandleIndexAt(ImVec2 pt, ImVec2 rmin, ImVec2 rmax, float hs) {
    ImVec2 c[8];
    HandleCorners(rmin, rmax, c);
    static const int grow[5] = { 2, 3, 4, 5, 6 };
    for (int i = 0; i < 5; i++) {
        int h = grow[i];
        if (fabsf(pt.x - c[h].x) <= hs && fabsf(pt.y - c[h].y) <= hs) return h;
    }
    return -1;
}
// tutamaç → imleç eşlemesi (M2God: boyutlandırma imleçleri)
static ImGuiMouseCursor CursorOfHandle(int i) {
    switch (i) {
    case 0: case 4: return ImGuiMouseCursor_ResizeNWSE;
    case 1: case 5: return ImGuiMouseCursor_ResizeNS;
    case 2: case 6: return ImGuiMouseCursor_ResizeNESW;
    case 3: case 7: return ImGuiMouseCursor_ResizeEW;
    }
    return ImGuiMouseCursor_Arrow;
}
// ---------------------------------------------------------------------------
//  TİP YETENEĞİNE GÖRE TUTAMAÇ MASKESİ (bit i = tutamaç i etkin):
//  FULL → 8 tutamaç; W → yalnız sağ/sol kenar (genişlik); AUTO → tutamaç yok.
//  "Görünmeyen boyutu" tutamaçla vaat etmeyiz: ImGui'de yüksekliği sabit olan
//  denetimlerde dikey tutamaç hiç gösterilmez (hayalet boyut hatasının kökü budur).
static unsigned HandleMaskOf(const Ctrl* c) {
    switch (SizeCapOf(c)) {
    case SIZECAP_FULL: return 0xFFu;
    case SIZECAP_W:    return (1u << 3) | (1u << 7);   // 3:sağ 7:sol kenar
    default:           return 0u;                     // içerik boyutlu — tutamaç yok
    }
}
#define FORM_HANDLE_MASK ((1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<6))  // form: sağ/alt büyütme
// sürükleme sırasında uygulanacak tutamaç (nesne form-göreli)
static void ApplyHandle(Ctrl* c, int handle, ImVec2 d) {
    int cap = SizeCapOf(c);
    int nx = c->x, ny = c->y, nw = s_origW, nh = s_origH;
    if (handle == 0 || handle == 6 || handle == 7) { nw = s_origW - (int)d.x; nx = s_origX + (int)d.x; }
    if (handle == 0 || handle == 1 || handle == 2) { nh = s_origH - (int)d.y; ny = s_origY + (int)d.y; }
    if (handle == 2 || handle == 3 || handle == 4) nw = s_origW + (int)d.x;
    if (handle == 4 || handle == 5 || handle == 6) nh = s_origH + (int)d.y;
    // tip farkındalı minimumlar: Separator ince çizgi olabilir (2px); diğer
    // FULL tiplerde anlamsız küçük kutu engellenir
    int minH = (c->type == "Separator") ? 2 : 8;
    nw = ImMax(nw, cap == SIZECAP_FULL ? 12 : 24);
    if (cap == SIZECAP_FULL) nh = ImMax(nh, minH);
    if (g_snapEnabled) {
        nx = (nx + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        ny = (ny + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        nw = (nw + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        nh = (nh + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
    }
    c->x = nx; c->y = ny; c->w = nw; c->h = nh;
    // BOYUT↔ÖZELLİK SENKRONU: tutamaçla değişince Width/Height/SizeArg/HeightItems
    // de aynı sayıyı gösterir — özellik paneli ve kod üretimi daima görüntüyle aynı.
    SyncSizeProps(c, nw, nh);
    if (c->type == "ListBox") {
        float lh = ImGui::GetTextLineHeightWithSpacing();
        float fp = ImGui::GetStyle().FramePadding.y * 2.0f;
        int items = (int)((nh - fp) / lh - 0.25f + 0.5f);
        if (items < 1) items = 1;
        c->props["HeightItems"] = std::to_string(items);
    }
    g_modelDirty = true;
}
static void ApplyFormHandle(Model& m, int handle, ImVec2 d) {
    // HATA DÜZELTİLDİ (sınır tanımayan büyüme): boyut artık TUTAMAÇ YAKALANDIĞI
    // andaki boyuttan (s_origX/s_origY) hesaplanır. Eski kod her karede GÜNCEL
    // formW/formH üzerine TOPLAM delta'yı ekliyordu → fare basılı kaldığı sürece
    // form durmadan büyüyordu (her kare birikerek). Artık durgun fare = değişim yok.
    int nw = s_origX, nh = s_origY;
    if (handle == 2 || handle == 3 || handle == 4) nw += (int)d.x;
    if (handle == 4 || handle == 5 || handle == 6) nh += (int)d.y;
    nw = ImMax(nw, 120); nh = ImMax(nh, 80);
    // canvas sınırı: form tasarım yüzeyinden TAŞAMAZ (panelleri örten mavi ekran hatası)
    int maxW = (int)(s_canvasMax.x - s_canvasMin.x) - 80;    // 40 sol + 40 sağ pay
    int maxH = (int)(s_canvasMax.y - s_canvasMin.y) - 130;   // 30 üst + tepsi(46) + pay
    if (maxW > 120) nw = ImMin(nw, maxW);
    if (maxH > 80)   nh = ImMin(nh, maxH);
    if (g_snapEnabled) {
        nw = (nw + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        nh = (nh + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
    }
    m.formW = nw; m.formH = nh;
    m.formProps["ClientSize"] = std::to_string(m.formW) + ", " + std::to_string(m.formH);
    g_modelDirty = true;
}

// taşma kırpma göstergesi (görünür boyut canvas'a sığmadı mı)
static bool s_formClamped = false;

// seçim çerçevesi + tutamaçlar çiz (foreground — her şeyin üstünde)
// mask: çizilecek tutamaçlar (bit i) — 0 = yalnız çerçeve, tutamaç yok
static void DrawSelFrame(ImDrawList* fg, ImVec2 rmin, ImVec2 rmax, bool primary, bool formFrame, unsigned mask = 0xFFu) {
    ImU32 accent = primary ? IM_COL32(255, 176, 32, 255) : IM_COL32(255, 200, 80, 235);
    fg->AddRect(ImVec2(rmin.x - 2, rmin.y - 2), ImVec2(rmax.x + 2, rmax.y + 2), accent, 0.0f, 0, primary ? 1.5f : 1.8f);
    if (!primary) return;
    ImVec2 c[8];
    HandleCorners(rmin, rmax, c);
    float hs = 4.5f;
    for (int i = 0; i < 8; i++) {
        if (!(mask & (1u << i))) continue;   // tip bu yönde boyutlanamaz — tutamaç yok
        fg->AddRectFilled(ImVec2(c[i].x - hs, c[i].y - hs), ImVec2(c[i].x + hs, c[i].y + hs), accent);
        fg->AddRect(ImVec2(c[i].x - hs, c[i].y - hs), ImVec2(c[i].x + hs, c[i].y + hs), IM_COL32(0, 0, 0, 255));
    }
    (void)formFrame;
}

// ---------------------------------------------------------------------------
//  ana çizim
void Draw(const ImVec2& rectMin, const ImVec2& rectMax) {
    s_canvasMin = rectMin; s_canvasMax = rectMax;
    s_hitRects.clear();
    Model& m = g_model;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mp = io.MousePos;

    // ================= 1) canvas zemin penceresi ============================
    {
        ImGui::SetNextWindowPos(rectMin);
        ImGui::SetNextWindowSize(ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##canvas_host", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
        ImGui::PopStyleVar(2);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(rectMin, rectMax, IM_COL32(41, 41, 43, 255));   // nötr gri (mavimsi ton kaldırıldı)
        if (g_gridVisible) {
            ImU32 dot = IM_COL32(94, 94, 98, 200);
            for (float x = rectMin.x; x < rectMax.x; x += GRID_SIZE)
                for (float y = rectMin.y; y < rectMax.y; y += GRID_SIZE)
                    dl->AddCircleFilled(ImVec2(x, y), 0.7f, dot, 4);
        }
        s_hostHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        ImGui::End();
    }

    // ================= 2) tasarlanan form — ÜRETİM temasıyla =================
    std::string title = m.formProps.count("Text") ? m.formProps["Text"] : "Form1";
    if (title.empty()) title = "Form1";   // boş başlık → ImGui assert; daima dolu
    ImGuiWindowFlags flags = FormWinFlagsOf(m);
    ImVec2 formPos(rectMin.x + 40.0f, rectMin.y + 30.0f);
    s_formPos = formPos;
    s_formSize = ImVec2((float)m.formW, (float)m.formH);

    theme::PushProduction(codegen::g_settings.themeId);   // WYSIWYG: üretim teması
    // form düzeyi stil: WindowBg / Rounding / Border (props)
    int nCol = 0, nVar = 0;
    {
        // BAŞLIK ÇUBUĞU NÖTR (mavi odak hatası): form içine tıklanınca ImGui formu
        // odaklı sayar ve başlığı TitleBgActive (koyu mavi) ile boyar — form
        // "seçilmiş" görünürdü. Tasarım yüzeyinde başlık daima nötr kalır.
        ImVec4 tb = ImGui::GetStyleColorVec4(ImGuiCol_TitleBg);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, tb);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, tb);
        nCol += 2;
        float f[4];
        if (objstyle::ParseHexColor(m.formProps.count("WindowBgColor") ? m.formProps["WindowBgColor"] : "", f)) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(f[0], f[1], f[2], f[3]));
            nCol++;
        }
        if (m.formProps.count("WindowRounding") && (float)atof(m.formProps["WindowRounding"].c_str()) >= 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, (float)atof(m.formProps["WindowRounding"].c_str()));
            nVar++;
        }
        if (m.formProps.count("WindowBorderSize") && (float)atof(m.formProps["WindowBorderSize"].c_str()) >= 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, (float)atof(m.formProps["WindowBorderSize"].c_str()));
            nVar++;
        }

        ImGui::SetNextWindowPos(formPos);
        // GÖRÜNÜR boyut: canvas'a sığmazsa görüntü alanı ile kırpılır (model boyutu
        // korunur — üretilen uygulama gerçek boyutu kullanır). Form, canvas dışına
        // taşıp panelleri örterek tüm ekranı kaplama hatası böyle kesin engellendi.
        ImVec2 drawSz((float)m.formW, (float)m.formH);
        float availW = (rectMax.x - rectMin.x) - 80.0f;    // 40 sol + 40 sağ pay
        float availH = (rectMax.y - rectMin.y) - 130.0f;   // 30 üst + tepsi(46) + pay
        s_formClamped = false;
        if (availW > 200.0f && drawSz.x > availW) { drawSz.x = availW; s_formClamped = true; }
        if (availH > 200.0f && drawSz.y > availH) { drawSz.y = availH; s_formClamped = true; }
        ImGui::SetNextWindowSize(drawSz);
        bool open = ImGui::Begin(title.c_str(), nullptr, flags);
        if (open) {
            // menü (varsa) — üretilen kodla aynı Menus/Commands mantığı
            for (Ctrl* c : m.ctrls) if (c->type == "Menu") {
                if (ImGui::BeginMenuBar()) {
                    for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Dosya|Düzen|Yardım")), L'|')) {
                        std::string men = WideStrToUtf8(s);
                        if (ImGui::BeginMenu(men.c_str())) {
                            for (auto& cmd : SplitW(Utf8ToWideStr(c->Get("Commands", "")), L'|')) {
                                std::string cline = WideStrToUtf8(cmd);
                                size_t col = cline.find(':');
                                if (col == std::string::npos) continue;
                                std::string owner = TrimA(cline.substr(0, col));
                                std::string item = TrimA(cline.substr(col + 1));
                                if (owner == men && !item.empty()) ImGui::MenuItem((item + "##" + c->name).c_str());
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenuBar();
                }
            }
            // üst-seviye nesneler (parent=null) — z sırası: listede son = üstte
            // 6. TUR: form penceresinin klip yüzeyi — hit-test kırpma farkındalı.
            {
                ImGuiWindow* fw2 = ImGui::GetCurrentWindow();
                ImVec2 cm = fw2->ClipRect.Min, cx2 = fw2->ClipRect.Max;
                for (Ctrl* c : m.ctrls) if (!c->parent && c->type != "Menu") DrawCtrl(c, m, cm, cx2);
            }
        }
        ImGui::End();
        // Form her zaman canvas zemininin ÜSTÜNDE render edilsin:
        // ImGui pencere z-sırası odak/oluşturma sırasına göredir — formu
        // görüntü yığınının en öne taşı (host ızgarası/paneller örtmesin).
        // DİKKAT: AÇIK POPUP varken taşıma YAPILMAZ — aksi halde bağlam menüsü
        // formun ARKASINA girip yarı görünür kalırdı (z-sırası hatası).
        if (ImGuiWindow* fwz = ImGui::FindWindowByName(title.c_str())) {
            if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
                ImGui::BringWindowToDisplayFront(fwz);
        }

        if (nVar > 0) ImGui::PopStyleVar(nVar);
        if (nCol > 0) ImGui::PopStyleColor(nCol);
    }
    theme::PopProduction();
    // form penceresinin GERÇEK dikdörtgeni (başlık çubuğu dahil)
    {
        ImGuiWindow* fw = ImGui::FindWindowByName(title.c_str());
        if (fw) { s_formPos = fw->Pos; s_formSize = fw->Size; }
    }

    // ================= 3) bileşen tepsisi (görsel olmayan) ====================
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        float trayY = s_formPos.y + s_formSize.y + 14.0f;
        float trayX = s_formPos.x;
        dl->AddRectFilled(ImVec2(trayX, trayY), ImVec2(trayX + m.formW, trayY + 26), IM_COL32(34, 34, 36, 255), 3.0f);
        dl->AddText(ImVec2(trayX + 6, trayY + 6), IM_COL32(150, 152, 160, 255), "Bile\305\237enler:");
        float cx = trayX + 90;
        for (Ctrl* c : m.ctrls) {
            if (c->type == "Timer" || c->type == "FontRef" || c->type == "Popup") {
                std::string lbl = "[ " + c->name + " ]";
                ImVec2 ts = ImGui::CalcTextSize(lbl.c_str());
                dl->AddRectFilled(ImVec2(cx, trayY + 3), ImVec2(cx + ts.x + 10, trayY + 23), IM_COL32(56, 56, 58, 255), 3.0f);
                dl->AddText(ImVec2(cx + 5, trayY + 6), IM_COL32(200, 202, 210, 255), lbl.c_str());
                WidgetRect r; r.c = c;
                r.rmin = ImVec2(cx, trayY + 3); r.rmax = ImVec2(cx + ts.x + 10, trayY + 23);
                r.tray = true;
                r.cclipMin = r.rmin; r.cclipMax = r.rmax;   // tepsi kendi yüzeyi
                s_hitRects.push_back(r);
                cx += ts.x + 16;
            }
        }
    }

    // ================= 4) etkileşim durumu makinesi ==========================

    // --- (0) bağlam menüsü/pencere açıksa canvas etkileşimleri KAPALI ---
    // (menü öğesine tıklamak tasarımda yanlışlıkla seçim/başlatma yapmasın)
    bool ctxOpen = ImGui::IsPopupOpen("##canvas_ctx");

    // --- (a) yerleştirme modu: form İÇİNE tıklama YERLEŞTİRİR (tek sefer!), ESC iptal ---
    if (!g_armTool.empty() && !ctxOpen) {
        bool inForm = mp.x >= s_formPos.x && mp.x <= s_formPos.x + s_formSize.x &&
                      mp.y >= s_formPos.y && mp.y <= s_formPos.y + s_formSize.y;
        if (ImGui::IsMouseClicked(0) && inForm && !io.WantTextInput) {
            // içerideki konteyneri bul — GEÇ ÇİZİLEN (görsel olarak ÜSTTE) olan
            // (iç içe panellerde içteki DAHA GEÇ kayıtlanır → en içteki kazanır)
            Ctrl* parent = nullptr;
            ImVec2 rmin, rmax;
            float parentTopPad = 0.0f;
            for (auto& r : s_hitRects) {
                Ctrl* cc = r.c;
                if (r.tray || !IsContainerType(cc->type) || cc->nonVisual) continue;
                if (PtInRect(mp, r.rmin, r.rmax)) { parent = cc; rmin = r.rmin; rmax = r.rmax; parentTopPad = r.topPad; }
            }
            int px, py;
            if (parent) { px = (int)(mp.x - rmin.x); py = (int)(mp.y - (rmin.y + parentTopPad)); }
            else { px = (int)(mp.x - s_formPos.x); py = (int)(mp.y - s_formPos.y); }
            if (g_snapEnabled) { px = px / GRID_SIZE * GRID_SIZE; py = py / GRID_SIZE * GRID_SIZE; }
            Ctrl* added = g_model.Add(g_armTool, px, py, parent);
            if (added) {
                // TabBar içine yerleştirme → TabOwner = o anki AKTİF sekme başlığı
                // (eskiden TabOwner atanmazdı → nesne HİÇBİR sekmede görünmezdi)
                if (parent && parent->type == "TabBar") {
                    auto it = s_tabActiveOwner.find(parent);
                    if (it != s_tabActiveOwner.end() && added->Get("TabOwner", "").empty())
                        added->Set("TabOwner", it->second);
                }
                // HATA DÜZELTİLDİ (sınırsız oluşturma): M2God/VS tasarımcı davranışı —
                // TEK yerleştirme sonrası araç seçimi (arm) KALKAR. Artık her tıkta
                // yeni nesne oluşmaz; yeni nesne için araç kutusundan yeniden seçilir.
                ui_editor::StatusUpdate("Eklendi: " + added->name + " — araç seçimi kalktı (yeni nesne için yeniden seç)");
                CancelArm();
            }
        }
    }

    // --- (b) tutamaç yakalama (birincil seçim / form) ---
    Ctrl* primary = m.Primary();
    ImVec2 prMin, prMax;
    bool havePrimaryRect = false;
    if (primary) {
        for (auto& r : s_hitRects)
            if (r.c == primary && !r.tray) { prMin = r.rmin; prMax = r.rmax; havePrimaryRect = true; break; }
        if (!havePrimaryRect && primary->type != "Menu" && primary->type != "Timer" &&
            primary->type != "FontRef" && primary->type != "Popup") {
            // çizilmemiş olabilir (kapalı sekme) — form konumuyla doğru yedek
            prMin = ImVec2(s_formPos.x + CtrlAbsPos(primary).x, s_formPos.y + CtrlAbsPos(primary).y);
            prMax = ImVec2(prMin.x + (float)(primary->w ? primary->w : 60), prMin.y + (float)(primary->h ? primary->h : 20));
            havePrimaryRect = true;
        } else if (!havePrimaryRect) {
            // tepsi nesnesi (Timer/FontRef/Popup): tepsiden bul
            for (auto& r : s_hitRects)
                if (r.c == primary) { prMin = r.rmin; prMax = r.rmax; havePrimaryRect = true; break; }
        }
    }
    bool haveFormRect = (m.formSelected && !m.selList.empty() ? false : true);

    if (ImGui::IsMouseReleased(0)) {
        // 6. TUR: gövde sürüklemesi BİTTİ — sürükle-bırak reparent'i çalıştır
        // (WinForms/M2God: nesneyi groupbox içine bırakınca ÇOCUĞU olur).
        if (s_dragBody) ReparentAfterDrag();
        s_dragBody = false;
        if (s_dragHandle >= 0) s_dragHandle = -1;
        s_formDrag = false;
        s_formHandle = -1;
    }

    // tutamaç üzerinde imleç + tık yakalama (yalnız TİPİN BOYUTLANABİLDİĞİ kenarlar)
    float hs = 5.5f;
    if (havePrimaryRect && s_dragHandle < 0 && !ctxOpen) {
        int hi = HandleIndexAt(mp, prMin, prMax, hs, HandleMaskOf(primary));
        if (hi >= 0) {
            ImGui::SetMouseCursor(CursorOfHandle(hi));
            if (ImGui::IsMouseClicked(0)) {
                s_dragHandle = hi;
                g_model.PushUndo();
                s_origX = primary->x; s_origY = primary->y; s_origW = primary->w; s_origH = primary->h;
                s_dragStart = mp;
            }
        }
    }
    // form seçili + form tutamaçları (yalnız SAĞ/ALT büyütme kenarları)
    if (m.formSelected && s_dragHandle < 0 && !s_formDrag && !ctxOpen) {
        int hi = FormHandleIndexAt(mp, s_formPos, ImVec2(s_formPos.x + s_formSize.x, s_formPos.y + s_formSize.y), hs);
        if (hi >= 0) {
            ImGui::SetMouseCursor(CursorOfHandle(hi));
            if (ImGui::IsMouseClicked(0)) {
                s_formHandle = hi;
                s_formDrag = true;
                g_model.PushUndo();
                s_origX = m.formW; s_origY = m.formH;
                s_dragStart = mp;
            }
        }
    }
    // sürükleme devamı
    if (s_dragHandle >= 0 && ImGui::IsMouseDown(0) && primary) {
        ImGui::SetMouseCursor(CursorOfHandle(s_dragHandle));
        ApplyHandle(primary, s_dragHandle, ImVec2(mp.x - s_dragStart.x, mp.y - s_dragStart.y));
    }
    if (s_formDrag && ImGui::IsMouseDown(0) && s_formHandle >= 0) {
        ImGui::SetMouseCursor(CursorOfHandle(s_formHandle));
        ApplyFormHandle(m, s_formHandle, ImVec2(mp.x - s_dragStart.x, mp.y - s_dragStart.y));
    }

    // --- (c) gövde tıklaması: seçim + grup taşıma başlat ---
    if (ImGui::IsMouseClicked(0) && s_dragHandle < 0 && !s_formDrag && g_armTool.empty() && !ctxOpen) {
        WidgetRect* hit = HitUnder(mp);
        if (hit) {
            bool additive = io.KeyShift || io.KeyCtrl;
            bool alreadySel = g_model.IsSelected(hit->c);
            if (!alreadySel) Select(hit->c, additive);
            else if (additive) { /* toggle yok — taşımaya izin ver */ }
            // taşıma: seçimde olan nesne → grup sürükleme başlat
            if (g_model.IsSelected(hit->c)) {
                s_dragBody = true;
                g_model.PushUndo();
                s_dragOrig.clear();
                s_dragRoots.clear();
                // yalnız SEÇİLİ KÖKLER taşınır: ebeveyni de seçiliyse çocuk atlanır —
                // yoksa çocuk hem kendi hem ebeveyni üzerinden İKİ kez hareket ederdi
                for (Ctrl* c : g_model.ValidSelection()) {
                    bool ancestorSel = false;
                    for (Ctrl* p = c->parent; p; p = p->parent)
                        if (g_model.IsSelected(p)) { ancestorSel = true; break; }
                    if (!ancestorSel) { s_dragOrig[c] = { c->x, c->y }; s_dragRoots.push_back(c); }
                }
                s_dragStart = mp;
            }
        } else if (PtInRect(mp, rectMin, rectMax)) {
            // form gövdesi VEYA canvas boşluğu → rubber band başlat.
            // HATA DÜZELTİLDİ (kullanıcı: "formun boş alanına tıklayınca formu
            // seçmiyor, ESC'yle seçiyor"): eski koşul s_hostHovered (canvas host
            // penceresinin hover'ı) istiyordu — tasarım formu host'un ÜSTÜNDE
            // çizildiğinden form üzerinde hover hep false'tu; tık hiç yakalanmıyor,
            // form seçilmiyordu. Artık nokta canvas dikdörtgeni içindeyse (form
            // dahil) yakalanır: tek tık = form seçimi, sürükleme = toplu seçim.
            s_band = true;
            s_bandStart = mp;
            s_bandCur = mp;
        }
    }

    // --- (d) rubber band sürükleme ---
    if (s_band) {
        if (ImGui::IsMouseDown(0)) s_bandCur = mp;
        else {
            // bırakıldı: alan içinde kalanları seç
            ImVec2 bmin(ImMin(s_bandStart.x, s_bandCur.x), ImMin(s_bandStart.y, s_bandCur.y));
            ImVec2 bmax(ImMax(s_bandStart.x, s_bandCur.x), ImMax(s_bandStart.y, s_bandCur.y));
            bool tiny = (bmax.x - bmin.x < 4 && bmax.y - bmin.y < 4);
            if (tiny) {
                SelectForm();      // tık = seçim temizle
            } else {
                std::vector<Ctrl*> sel;
                std::set<Ctrl*> seen;
                for (auto& r : s_hitRects) {
                    if (r.tray) continue;
                    // 6. TUR: klip-yönlü ETKİN dikdörtgen — konteyner dışına taşan
                    // görünmez çocuk band yakalamaz (kaybolmuş seçim hayaleti yok)
                    ImVec2 emin, emax;
                    if (!EffectiveRect(r, emin, emax)) continue;
                    // kesişim testi
                    if (emin.x <= bmax.x && emax.x >= bmin.x && emin.y <= bmax.y && emax.y >= bmin.y) {
                        if (seen.insert(r.c).second) sel.push_back(r.c);
                    }
                }
                // alt kontrolleri seçimden düşür (ebeveynleri de seçildiyse)
                std::vector<Ctrl*> eff;
                for (Ctrl* c : sel) {
                    bool ancestorSel = false;
                    for (Ctrl* p = c->parent; p; p = p->parent)
                        if (seen.count(p)) { ancestorSel = true; break; }
                    if (!ancestorSel) eff.push_back(c);
                }
                g_model.SetSelection(eff);
            }
            s_band = false;
        }
    }

    // --- (e) grup taşıma devamı ---
    if (s_dragBody && ImGui::IsMouseDown(0)) {
        ImVec2 d(mp.x - s_dragStart.x, mp.y - s_dragStart.y);
        for (auto& kv : s_dragOrig) {
            Ctrl* c = kv.first;
            int nx = kv.second.first + (int)d.x, ny = kv.second.second + (int)d.y;
            if (g_snapEnabled) { nx = (nx + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE; ny = (ny + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE; }
            c->x = nx; c->y = ny;
        }
        g_modelDirty = true;
    }

    // --- (f) SAĞ TIK: bağlam menüsü (M2God/VS tarzı — nesne üstünde
    //     Kes/Kopyala/Yapıştır/Sil/ÖNE GETİR/Arkaya Gönder; formda Yapıştır/Tümünü Seç)
    //     Yerleştirme modundayken sağ tık = iptal. ---
    if (!ctxOpen && ImGui::IsMouseReleased(1)) {
        if (!g_armTool.empty()) {
            CancelArm();
            ui_editor::StatusUpdate("Yerleştirme iptal edildi (sağ tık)");
        } else {
            WidgetRect* hit = HitUnder(mp);
            if (hit) {
                if (!g_model.IsSelected(hit->c)) Select(hit->c, false);   // sağ tık seçer (VS davranışı)
                s_ctxTarget = hit->c;
            } else {
                s_ctxTarget = nullptr;                                     // form / canvas menüsü
            }
            s_ctxRequest = true;
        }
    }

    // ================= 5) seçim vurguları (foreground) =======================
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    // ikincil seçimler: in çerçeve
    for (Ctrl* c : g_model.ValidSelection()) {
        if (c == primary) continue;
        for (auto& r : s_hitRects)
            if (r.c == c && !r.tray) { DrawSelFrame(fg, r.rmin, r.rmax, false, false); break; }
    }
    // birincil seçim: dolu tutamaçlı çerçeve (tip yeteneğine göre tutamaç maskesi)
    if (havePrimaryRect) DrawSelFrame(fg, prMin, prMax, true, false, HandleMaskOf(primary));
    // form seçimi: form çerçevesi + köşe tutamaçları (sağ/alt büyütme kenarları)
    if (m.formSelected)
        DrawSelFrame(fg, s_formPos, ImVec2(s_formPos.x + s_formSize.x, s_formPos.y + s_formSize.y), true, true, FORM_HANDLE_MASK);

    // ================= 6) yerleştirme modu göstergesi ========================
    if (!g_armTool.empty()) {
        fg->AddRect(rectMin, rectMax, IM_COL32(90, 200, 120, 200), 4.0f, 0, 2.0f);
        char b[160];
        snprintf(b, sizeof b, "YERLE\305\236T\304\260R: %s  (forma t\304\261kla — TEK yerle\305\237tirme · ESC/sa\304\237 t\304\261k iptal)", g_armTool.c_str());
        ImVec2 ts = ImGui::CalcTextSize(b);
        fg->AddRectFilled(ImVec2(rectMin.x + 8, rectMax.y - ts.y - 12), ImVec2(rectMin.x + 16 + ts.x, rectMax.y - 8), IM_COL32(20, 60, 30, 220), 3);
        fg->AddText(ImVec2(rectMin.x + 12, rectMax.y - ts.y - 8), IM_COL32(120, 255, 150, 255), b);
    }

    // rubber band çizgisi (kesikli dikdörtgen)
    if (s_band) {
        ImVec2 bmin(ImMin(s_bandStart.x, s_bandCur.x), ImMin(s_bandStart.y, s_bandCur.y));
        ImVec2 bmax(ImMax(s_bandStart.x, s_bandCur.x), ImMax(s_bandStart.y, s_bandCur.y));
        for (float x = bmin.x; x < bmax.x; x += 6.0f) {
            fg->AddLine(ImVec2(x, bmin.y), ImVec2(ImMin(x + 3, bmax.x), bmin.y), IM_COL32(255, 176, 32, 220), 1.0f);
            fg->AddLine(ImVec2(x, bmax.y), ImVec2(ImMin(x + 3, bmax.x), bmax.y), IM_COL32(255, 176, 32, 220), 1.0f);
        }
        for (float y = bmin.y; y < bmax.y; y += 6.0f) {
            fg->AddLine(ImVec2(bmin.x, y), ImVec2(bmin.x, ImMin(y + 3, bmax.y)), IM_COL32(255, 176, 32, 220), 1.0f);
            fg->AddLine(ImVec2(bmax.x, y), ImVec2(bmax.x, ImMin(y + 3, bmax.y)), IM_COL32(255, 176, 32, 220), 1.0f);
        }
        char b[96];
        snprintf(b, sizeof b, "%d x %d", (int)(bmax.x - bmin.x), (int)(bmax.y - bmin.y));
        fg->AddText(ImVec2(bmin.x, bmin.y - 16), IM_COL32(255, 200, 120, 255), b);
    }

    // kırpıldı göstergesi: gerçek boyut görünürlükte kırpılmışsa yaz
    if (s_formClamped) {
        char cb[96];
        snprintf(cb, sizeof cb, "g\303\266r\303\274n\303\274r alan k\303\274\303\247\303\274ld\303\274 — ger\303\247ek boyut: %d x %d", m.formW, m.formH);
        ImVec2 ts = ImGui::CalcTextSize(cb);
        fg->AddRectFilled(ImVec2(s_formPos.x + 6, s_formPos.y + s_formSize.y - ts.y - 8),
                          ImVec2(s_formPos.x + 12 + ts.x, s_formPos.y + s_formSize.y - 4), IM_COL32(28, 30, 36, 220), 3);
        fg->AddText(ImVec2(s_formPos.x + 9, s_formPos.y + s_formSize.y - ts.y - 6), IM_COL32(255, 190, 90, 255), cb);
    }

    // ================= 7) SAĞ TIK BAĞLAM MENÜSÜ ==============================
    // (M2God/VS: nesne üstünde Kes/Kopyala/Yapıştır/Klonla/Sil/ÖNE GETİR/Arkaya
    //  Gönder; form/canvas boşluğunda Yapıştır/Tümünü Seç/Özellikler)
    {
        ImGui::SetNextWindowPos(s_canvasMin);
        ImGui::SetNextWindowSize(ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("##ctx_host", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();
        if (s_ctxRequest) { ImGui::OpenPopup("##canvas_ctx"); s_ctxRequest = false; }
        // hedef hâlâ modelde var mı? (aradan silme/undo geçebilir)
        Ctrl* tgt = nullptr;
        for (Ctrl* c : g_model.ctrls) if (c == s_ctxTarget) { tgt = s_ctxTarget; break; }
        if (ImGui::BeginPopup("##canvas_ctx")) {
            Model& mm = g_model;
            // NavEnableKeyboard kasıtlı kapalı olduğundan ImGui'nin kendi ESC-ile-
            // popup-kapama yolu çalışmaz — menü içinde biz kapatırız.
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) ImGui::CloseCurrentPopup();
            if (tgt) {
                ImGui::TextDisabled("%s : %s", tgt->name.c_str(), tgt->type.c_str());
                ImGui::Separator();
                if (ImGui::MenuItem("Kes", "Ctrl+X")) { mm.PushUndo(); mm.CutSelection(); ImGui::CloseCurrentPopup(); }
                if (ImGui::MenuItem("Kopyala", "Ctrl+C")) { mm.CopySelection(); ImGui::CloseCurrentPopup(); }
                ImGui::BeginDisabled(mm.clipboardJson.empty());
                if (ImGui::MenuItem("Yap\304\261\305\237t\304\261r", "Ctrl+V")) { mm.PushUndo(); mm.PasteClipboard(GRID_SIZE, GRID_SIZE); ImGui::CloseCurrentPopup(); }
                ImGui::EndDisabled();
                ImGui::Separator();
                if (ImGui::MenuItem("Klonla (kopyala+yap\304\261\305\237t\304\261r)")) {
                    mm.PushUndo(); mm.CopySelection(); mm.PasteClipboard(GRID_SIZE, GRID_SIZE);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Sil", "Del")) { mm.PushUndo(); mm.DeleteList(mm.ValidSelection()); ImGui::CloseCurrentPopup(); }
                ImGui::Separator();
                if (ImGui::MenuItem("\303\226ne Getir")) { mm.BringToFront(tgt); ImGui::CloseCurrentPopup(); }      // M2God: z-sırası öne
                if (ImGui::MenuItem("Arkaya G\303\266nder")) { mm.SendToBack(tgt); ImGui::CloseCurrentPopup(); }
                ImGui::Separator();
                if (ImGui::MenuItem("\303\226zellikler")) { Select(tgt, false); ImGui::CloseCurrentPopup(); }
            } else {
                ImGui::TextDisabled("Form: %s  (%d x %d)", title.c_str(), m.formW, m.formH);
                ImGui::Separator();
                ImGui::BeginDisabled(mm.clipboardJson.empty());
                if (ImGui::MenuItem("Yap\304\261\305\237t\304\261r", "Ctrl+V")) { mm.PushUndo(); mm.PasteClipboard(GRID_SIZE, GRID_SIZE); ImGui::CloseCurrentPopup(); }
                ImGui::EndDisabled();
                if (ImGui::MenuItem("T\303\274m\303\274n\303\274 Se\303\247", "Ctrl+A")) { mm.SelectAll(); ImGui::CloseCurrentPopup(); }
                ImGui::Separator();
                if (ImGui::MenuItem("\303\226zellikler")) { SelectForm(); ImGui::CloseCurrentPopup(); }
            }
            ImGui::EndPopup();
        }
        ImGui::End();
    }

    // ================= 8) ölü tampon temizliği ===============================
    // silinen Ctrl* adresleri s_textBufs/s_lastVal içinde kalıntı bırakmasın
    // (yeni nesne aynı adrese gelirse eski metne miras kalırdı).
    {
        std::set<const Ctrl*> alive(m.ctrls.begin(), m.ctrls.end());
        for (auto it = s_textBufs.begin(); it != s_textBufs.end(); )
            if (!alive.count(it->first)) it = s_textBufs.erase(it); else ++it;
        for (auto it = s_lastVal.begin(); it != s_lastVal.end(); )
            if (!alive.count(it->first)) it = s_lastVal.erase(it); else ++it;
    }

    // canvas çerçevesi
    fg->AddRect(rectMin, rectMax, IM_COL32(22, 22, 24, 255), 0.0f, 0, 1.0f);
    (void)haveFormRect;
}

// ---------------------------------------------------------------------------
//  selftest/otomasyon kancaları: gerçek çizim dikdörtgenleri
void GetFormRect(float* x, float* y, float* w, float* h) {
    if (x) *x = s_formPos.x;
    if (y) *y = s_formPos.y;
    if (w) *w = s_formSize.x;
    if (h) *h = s_formSize.y;
}
bool ObjRectOf(const std::string& name, float* x, float* y, float* w, float* h) {
    for (auto& r : s_hitRects) {
        if (r.tray || r.c->name != name) continue;
        if (x) *x = r.rmin.x;
        if (y) *y = r.rmin.y;
        if (w) *w = r.rmax.x - r.rmin.x;
        if (h) *h = r.rmax.y - r.rmin.y;
        return true;
    }
    return false;
}

} // namespace canvas
