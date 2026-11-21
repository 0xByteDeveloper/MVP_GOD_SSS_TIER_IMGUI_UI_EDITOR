// surface.cpp — tasarım yüzeyi (parent-child, tıkla-yerleştir, bağlam menüsü)
#include "surface.h"
#include "app.h"
#include "factory.h"
#include "toolbox.h"
#include "theme.h"    // FAZ 2.9.1: nesne özel tema
#include "paintctl.h" // FAZ 2.12: nesne özel tema yerinde deri değişimi
#include <commctrl.h>

namespace surface {

// --- durum -------------------------------------------------------------------
static int  s_scrollX = 0, s_scrollY = 0;
static const int PAD = 12;          // form ile yüzey kenarı arası
static const int TITLE_H = 32;      // sahte başlık çubuğu (varsayılan)
static const int TRAY_H = 34;       // bileşen tepsisi
static Ctrl*    s_drag = nullptr;   // taşınan/boyutlanan
static int      s_dragMode = 0;     // 0 yok, 1 taşı, 2 boyutla
static int      s_handle = -1;      // 0..7
static POINT    s_start;            // fare başlangıç (yüzey client)
static RECT     s_startRect;        // kontrol başlangıç rect (parent görelisi)
static bool     s_started = false;  // eşik geçildi mi (undo için)
static bool     s_gridSnap = true;

// FAZ 2.10: WM_CTLCOLOR fırça önbelleği — dosya kapsamına taşındı (yok edişte
// temizlenir; HWND geri dönüşümünde bayat fırça döndürmesin)
static std::map<HWND, std::pair<COLORREF, HBRUSH>> s_brushCache;

// FAZ 2.10 ÇÖKME DÜZELTMESİ: tasarımcı alt sınıfama KOMUTLA yapılır.
// comctl32'nin SetWindowSubclass'ı süreç başında comctl32 v5'e bağlanır; Modern
// temada (comctl32 v6 aktifken) yaratılan SysListView32/SysTabControl32 gibi
// kontroller İÇSELDE v6'nın kendi alt sınıf zincirini kullanır — v5 tablolarını
// o pencereye uygulamak zinciri bozuyor ve pencere yok edilirken (tema geri
// dönüşünde) erişim ihlaline yol açıyordu (Modern→Oldschool çökmesi).
// Saf SetWindowLongPtrW + eski proc pencere ÖZELLİĞİNDE (M2OLD_DESIGN):
// kullanıcı32'yi kullanır, comctl32 sürümünden bağımsız — her temada güvenli.
static const wchar_t* kDesignOldProcProp = L"M2OLD_DESIGN";
static LRESULT CALLBACK DesignSubProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_NCHITTEST) return HTTRANSPARENT;   // fare yüzeye aksin
    WNDPROC old = (WNDPROC)GetPropW(h, kDesignOldProcProp);
    return old ? CallWindowProcW(old, h, m, w, l) : DefWindowProcW(h, m, w, l);
}
static POINT    s_lastCursor = { 0, 0 };  // son imleç (yüzey client) — bırakma noktası
static HWND     s_surf = nullptr;         // yüzey (drag katmanı için)
static HWND     s_dragLayer = nullptr;    // SÜRÜKLEME GÖRSELİ KATMANI (tüm çocukların üstünde)

// FAZ 2.8 — FORM BOYUTLANDIRMA: form seçiliyken kenarlarda 8 tutamaç;
// sürükleyince ClientSize (formW/formH) değişir. s_formX/s_formY formun yüzey
// üzerindeki konumudur (sol/üst kenar sürüklenince form kayar — VS davranışı).
static bool     s_formDrag = false;
static RECT     s_formStartRect;      // {x, y, x+w, y+h} — başlangıç form geometrisi
static int      s_formX = PAD, s_formY = PAD;

// --- sürükleme görsel katmanı -------------------------------------------------
// Gerçek HWND'ler sürükleme sırasında HİZLETİLMEZ (hayalet piksel/kendini
// kopyalamış gibi görünen bug'ların kök nedeni); yerine katman penceresi
// kontrollerin ÜSTÜNDE hayalet + bırakma hedefi çizer.
// Katman SetWindowRgn ile şekillendirilir: yalnız hayalet + hedef çerçevesi
// görünür, geri kalanı tamamen şeffaf ve tıklanabilir (Wine/Windows uyumlu).
static RECT CtrlRectOnSurface(const Ctrl* c, HWND surf);
static Ctrl* HitAt(HWND surf, int sx, int sy, Ctrl* exclude);
static Ctrl* DropParentAt(HWND surf, int sx, int sy, Ctrl* dragged);
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1);
static RECT ClientAreaRect(HWND surf);
// FAZ 2.6 ileri bildirimler
static bool SmartTagRect(HWND surf, Ctrl* c, RECT& out);
static void DrawSmartTag(HDC dc, HWND surf, Ctrl* c);
static LRESULT CALLBACK DragLayerProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NCHITTEST: return HTTRANSPARENT;   // fare alttaki yüzeye/ kontrollere gitsin
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        Ctrl* d = s_drag;
        if (d && s_dragMode == 1 && s_started) {
            // hayalet kontrol (bölge zaten yalnız bu alanları kapsıyor)
            RECT g = CtrlRectOnSurface(d, s_surf);
            HBRUSH fb = CreateSolidBrush(RGB(204, 232, 255));
            FillRect(dc, &g, fb); DeleteObject(fb);
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
            HPEN op = (HPEN)SelectObject(dc, pen);
            HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, g.left, g.top, g.right, g.bottom);
            // seçim kesikli çerçeve
            HPEN dpen = CreatePen(PS_DOT, 1, RGB(0, 90, 158));
            HPEN op2 = (HPEN)SelectObject(dc, dpen);
            Rectangle(dc, g.left - SEL_MARGIN, g.top - SEL_MARGIN, g.right + SEL_MARGIN, g.bottom + SEL_MARGIN);
            SelectObject(dc, op2); DeleteObject(dpen);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, g_fontUI);
            SetTextColor(dc, RGB(0, 60, 110));
            RECT tr = g; tr.left += 6;
            std::wstring label = d->name + L" (" + d->type + L")";
            DrawTextW(dc, label.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            // bırakma hedefi çerçevesi (bölge yalnız şeritleri kapsar)
            Ctrl* tgt = DropParentAt(s_surf, s_lastCursor.x, s_lastCursor.y, d);
            if (tgt && tgt != d) {
                int x0, y0, x1, y1; ContainerChildArea(tgt, x0, y0, x1, y1);
                POINT ap = g_model.AbsPos(tgt);
                RECT cr = ClientAreaRect(s_surf);
                RECT t = { cr.left + ap.x + x0, cr.top + ap.y + y0, cr.left + ap.x + x1, cr.top + ap.y + y1 };
                HPEN tpen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
                HPEN otp = (HPEN)SelectObject(dc, tpen);
                HBRUSH obt = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
                Rectangle(dc, t.left, t.top, t.right, t.bottom);
                SelectObject(dc, otp); SelectObject(dc, obt); DeleteObject(tpen);
            }
        }
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

// katmanın görünür bölgesi = hayalet (+kenar) + hedef çerçeve şeritleri
static void DragLayerUpdateRegion(HWND surf) {
    if (!s_dragLayer || !IsWindow(s_dragLayer)) return;
    if (!(s_drag && s_dragMode == 1 && s_started)) { SetWindowRgn(s_dragLayer, nullptr, TRUE); return; }
    RECT g = CtrlRectOnSurface(s_drag, surf);
    InflateRect(&g, SEL_MARGIN + 1, SEL_MARGIN + 1);
    HRGN rgn = CreateRectRgnIndirect(&g);
    Ctrl* tgt = DropParentAt(surf, s_lastCursor.x, s_lastCursor.y, s_drag);
    if (tgt && tgt != s_drag) {
        int x0, y0, x1, y1; ContainerChildArea(tgt, x0, y0, x1, y1);
        POINT ap = g_model.AbsPos(tgt);
        RECT cr = ClientAreaRect(surf);
        RECT t = { cr.left + ap.x + x0, cr.top + ap.y + y0, cr.left + ap.x + x1, cr.top + ap.y + y1 };
        HRGN ss[4] = {
            CreateRectRgn(t.left - 1, t.top - 1, t.right + 1, t.top + 1),
            CreateRectRgn(t.left - 1, t.bottom - 1, t.right + 1, t.bottom + 1),
            CreateRectRgn(t.left - 1, t.top, t.left + 1, t.bottom),
            CreateRectRgn(t.right - 1, t.top, t.right + 1, t.bottom)
        };
        for (auto s : ss) { CombineRgn(rgn, rgn, s, RGN_OR); DeleteObject(s); }
    }
    SetWindowRgn(s_dragLayer, rgn, TRUE);
}

static void DragLayerShow(HWND surf) {
    if (!s_dragLayer) return;
    RECT rc; GetClientRect(surf, &rc);
    MoveWindow(s_dragLayer, 0, 0, rc.right, rc.bottom, FALSE);
    SetWindowPos(s_dragLayer, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    DragLayerUpdateRegion(surf);
    ShowWindow(s_dragLayer, SW_SHOWNA);
    InvalidateRect(s_dragLayer, nullptr, TRUE);
}
static void DragLayerHide() {
    if (s_dragLayer && IsWindow(s_dragLayer)) ShowWindow(s_dragLayer, SW_HIDE);
}
static void DragLayerInvalidate() {
    if (s_dragLayer && IsWindow(s_dragLayer)) {
        DragLayerUpdateRegion(s_surf);
        InvalidateRect(s_dragLayer, nullptr, TRUE);
    }
}

// --- koordinat yardımcıları ----------------------------------------------------
static void RepaintAll(HWND surf);   // ileri bildirim (DoScroll da kullanır)

// FAZ 2.8: FormBorderStyle=None → form başlıksız çizilir (derlenmiş exe ile
// görsel uyum; VS davranışı). Diğer stiller başlık çubuğu gösterir.
static int TitleH() {
    auto it = g_model.formProps.find(L"FormBorderStyle");
    if (it != g_model.formProps.end() && _wcsicmp(it->second.c_str(), L"None") == 0)
        return 0;
    return TITLE_H;
}

// formun İSTEMCİ alanının yüzey koordinatındaki dikdörtgeni (kaydırma dahil)
static RECT ClientAreaRect(HWND surf) {
    RECT f = FormRect(surf);
    int th = TitleH();
    return { f.left, f.top + th, f.right, f.top + th + g_model.formH };
}
static RECT TrayRect(HWND surf) {
    // bileşen tepsisi İSTEMCİ ALANIN ALTINDA (VS davranışı — client'ı yemez)
    RECT c = ClientAreaRect(surf);
    return { c.left, c.bottom, c.right, c.bottom + TRAY_H };
}
void ScreenToForm(HWND surf, int sx, int sy, int* fx, int* fy) {
    RECT c = ClientAreaRect(surf);
    *fx = sx - c.left;
    *fy = sy - c.top;
}
// kontrolün form koordinatındaki rect'i (parent zinciri toplanır)
static RECT AbsRect(const Ctrl* c) {
    POINT p = g_model.AbsPos(c);
    return { p.x, p.y, p.x + c->w, p.y + c->h };
}
// kontrolün yüzey client koordinatındaki rect'i
static RECT CtrlRectOnSurface(const Ctrl* c, HWND surf) {
    RECT cr = ClientAreaRect(surf);
    POINT p = g_model.AbsPos(c);
    return { cr.left + p.x, cr.top + p.y, cr.left + p.x + c->w, cr.top + p.y + c->h };
}
static bool VisibleCtrl(Ctrl* c) {
    return !c->nonVisual && _wcsicmp(c->Get(L"Visible", L"True").c_str(), L"true") == 0;
}

// handle dikdörtgenleri (yüzey koord), seçim rect'e göre
static RECT HandleRect(const RECT& sel, int idx) {
    int cx = (sel.left + sel.right) / 2, cy = (sel.top + sel.bottom) / 2;
    POINT p[8] = {
        { sel.left, sel.top }, { cx, sel.top }, { sel.right, sel.top },
        { sel.right, cy }, { sel.right, sel.bottom }, { cx, sel.bottom },
        { sel.left, sel.bottom }, { sel.left, cy }
    };
    int hs = 7;
    RECT r = { p[idx].x - hs / 2, p[idx].y - hs / 2, p[idx].x - hs / 2 + hs, p[idx].y - hs / 2 + hs };
    return r;
}

RECT FormRect(HWND surf) {
    // FAZ 2.8: form = başlık (stil None ise yok) + İSTEMCİ (formW×formH) + tepsi
    RECT f = { s_formX - s_scrollX, s_formY - s_scrollY,
               s_formX - s_scrollX + g_model.formW,
               s_formY - s_scrollY + TitleH() + g_model.formH + TRAY_H };
    return f;
}

// VS kuralı: konteyner üstüne bırakılırsa içine; düz kontrol üstüne bırakılırsa
// o kontrolün bulunduğu katmana girer.
// TabControl: yalnızca SAYFA ALANI (sekme şeridi değil) içine alır — şeride
// bırakılanlar dış katmana gider (yanlışlıkla yutma sorununu önler).
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1);
static Ctrl* DropParentAt(HWND surf, int sx, int sy, Ctrl* dragged) {
    Ctrl* t = HitAt(surf, sx, sy, dragged);
    Ctrl* p = nullptr;
    if (t) {
        if (IsContainerType(t->type)) {
            if (_wcsicmp(t->type.c_str(), L"TabControl") == 0) {
                int x0, y0, x1, y1; ContainerChildArea(t, x0, y0, x1, y1);
                int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
                POINT ap = g_model.AbsPos(t);
                int lx = fx - ap.x, ly = fy - ap.y;
                if (lx < x0 || ly < y0 || lx > x1 || ly > y1) p = t->parent;
                else p = t;
            } else p = t;
        }
        else p = t->parent;
    }
    if (p && (p == dragged || g_model.IsAncestorOf(dragged, p))) p = dragged->parent;
    return p;
}

// --- hit-test (parent-child duyarlı) ---------------------------------------------
// lx,ly: parent'in sol üstüne göre (parent=null → form koordinatı)
static Ctrl* HitInEx(Ctrl* parent, int lx, int ly, Ctrl* exclude) {
    auto kids = g_model.ChildrenOf(parent);
    // FAZ 2.6: TabControl çocukları arasında SEÇİLİ sayfa en üstte sayılır
    // (sayfalar aynı dikdörtgende üst üste durur; VS davranışı: görünen sayfa kazanır)
    if (parent && _wcsicmp(parent->type.c_str(), L"TabControl") == 0) {
        std::wstring selName = parent->Get(L"SelectedTab");
        for (int i = (int)kids.size() - 1; i >= 0; i--) {
            Ctrl* k = kids[i];
            if (_wcsicmp(k->type.c_str(), L"TabPage") == 0 && _wcsicmp(k->name.c_str(), selName.c_str()) == 0) {
                kids.erase(kids.begin() + i);
                kids.push_back(k);
                break;
            }
        }
    }
    for (int i = (int)kids.size() - 1; i >= 0; i--) {
        Ctrl* k = kids[i];
        if (k == exclude) continue;
        if (!VisibleCtrl(k)) continue;
        RECT r = { k->x, k->y, k->x + k->w, k->y + k->h };
        if (!PtInRect(&r, { lx, ly })) continue;
        Ctrl* sub = HitInEx(k, lx - k->x, ly - k->y, exclude);
        return sub ? sub : k;      // çocuklar her zaman parent'ın üstündedir
    }
    return nullptr;
}
static Ctrl* HitAt(HWND surf, int sx, int sy, Ctrl* exclude = nullptr) {
    int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
    return HitInEx(nullptr, fx, fy, exclude);
}
static Ctrl* HitControl(HWND surf, int sx, int sy) { return HitAt(surf, sx, sy); }

// konteynerin "çocuk alanı": GroupBox başlığı, TabControl sekme şeridi hariç
static void ContainerChildArea(Ctrl* cont, int& x0, int& y0, int& x1, int& y1) {
    x0 = 0; y0 = 0; x1 = cont->w; y1 = cont->h;
    if (_wcsicmp(cont->type.c_str(), L"GroupBox") == 0) { y0 = 20; x0 = 4; x1 -= 4; y1 -= 4; }
    else if (_wcsicmp(cont->type.c_str(), L"TabControl") == 0) {
        y0 = 30; x0 = 4; x1 -= 4; y1 -= 4;
        if (cont->hwnd) {
            RECT dr = { 0, 0, cont->w, cont->h };
            SendMessageW(cont->hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&dr);
            x0 = max(0, (int)dr.left); y0 = max(0, (int)dr.top);
            x1 = min(cont->w, (int)dr.right); y1 = min(cont->h, (int)dr.bottom);
        }
    }
}
static void ClampIntoContainer(Ctrl* cont, int& px, int& py, int w, int h) {
    int x0, y0, x1, y1;
    ContainerChildArea(cont, x0, y0, x1, y1);
    px = max(x0, min(px, max(x0, x1 - w)));
    py = max(y0, min(py, max(y0, y1 - h)));
}

// --- scrollbar yönetimi -------------------------------------------------------
// içerik boyutu: formun (s_formX/s_formY konumuna göre) sağ/alt kenarı + PAD
static int ContentW() { return s_formX + g_model.formW + PAD; }
static int ContentH() { return s_formY + TitleH() + g_model.formH + TRAY_H + PAD; }
static void UpdateScroll(HWND surf) {
    RECT rc; GetClientRect(surf, &rc);
    int viewW = rc.right, viewH = rc.bottom;
    int contentW = ContentW();
    int contentH = ContentH();
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0; si.nMax = contentW;
    si.nPage = (UINT)max(1, viewW); si.nPos = s_scrollX;
    SetScrollInfo(surf, SB_HORZ, &si, TRUE);
    si.nMax = contentH; si.nPage = (UINT)max(1, viewH); si.nPos = s_scrollY;
    SetScrollInfo(surf, SB_VERT, &si, TRUE);
    s_scrollX = min(s_scrollX, max(0, contentW - (int)viewW));
    s_scrollY = min(s_scrollY, max(0, contentH - (int)viewH));
}

static void DoScroll(HWND surf, int dx, int dy) {
    int odx = s_scrollX, ody = s_scrollY;
    RECT rc; GetClientRect(surf, &rc);
    int maxSX = max(0, ContentW() - (int)rc.right);
    int maxSY = max(0, ContentH() - (int)rc.bottom);
    s_scrollX = min(max(0, s_scrollX + dx), maxSX);
    s_scrollY = min(max(0, s_scrollY + dy), maxSY);
    if (odx != s_scrollX || ody != s_scrollY) {
        ScrollWindow(surf, odx - s_scrollX, ody - s_scrollY, nullptr, nullptr);
        UpdateScroll(surf);
        RepaintAll(surf);   // kaydırmada da çocuklar dahil tam temizlik (hayalet olmasın)
    }
}

// --- çizim --------------------------------------------------------------------
// FAZ 2.13: temaya duyarlı başlık çubuğu:
//   Apple & Mac → macOS pencere: #ECECEC zemin, SOLDA trafik ışıkları
//                 (kırmızı/sarı/yeşil), başlık metni ışıkların sağında;
//                 sağda Windows kutucukları YOK (mac penceresinde yoktur)
//   M2God grafit → M2God TitleBar_Paint: TEMA GRADYANI (Ruby: kırmızı v.b.)
//                 + WhiteSmoke başlık + sağda tek X düğmesi (M2God btnClose
//                 teması gradyanlı 18×18 — ana programdaki gibi)
//   diğer       → eski açık gri Windows başlığı (Oldschool/Modern)
class TitleBarPalette { public: COLORREF bg, line, text, glyph, disabledFill, disabledLine; int kind; }; // kind 0=win 1=mac 2=grafit
class TitleBarGrad { public: COLORREF top, bot; };   // grafit başlık gradyanı (tbBase1/tbBase2)
static void DrawTitleBar(HDC dc, const RECT& f) {
    int th = TitleH();
    if (th <= 0) return;   // FAZ 2.8: FormBorderStyle=None → başlık yok (exe ile görsel uyum)
    int cur = theme::Current();
    TitleBarPalette p;
    if (cur == theme::APPLEMAC)       { p = { RGB(236,236,236), RGB(210,210,212), RGB(0,0,0),       RGB(60,60,60),    RGB(224,224,226), RGB(200,200,202), 1 }; }
    else if (theme::IsGraphite(cur))  { p = { RGB(37,37,37),    RGB(62,62,62),    RGB(245,245,245), RGB(245,245,245), RGB(45,45,45),    RGB(62,62,62),    2 }; }
    else                              { p = { RGB(247,247,247), RGB(213,213,213), RGB(0,0,0),       RGB(60,60,60),    RGB(238,238,238), RGB(192,192,192), 0 }; }
    RECT t = { f.left, f.top, f.right, f.top + th };
    if (p.kind == 2) {
        // M2God grafit başlık çubuğu — dikey tema gradyanı (TitleBar_Paint birebir:
        // Ruby (118,3,0)→(53,1,0), Yellow (117,81,0)→(56,38,0) v.b.)
        COLORREF top = theme::TitleGradTop(cur), bot = theme::TitleGradBottom(cur);
        int hgt = t.bottom - t.top;
        for (int y = t.top; y < t.bottom; y++) {
            int q = hgt > 0 ? (y - t.top) * 1000 / hgt : 0;
            COLORREF c = RGB(GetRValue(top) + (GetRValue(bot) - GetRValue(top)) * q / 1000,
                             GetGValue(top) + (GetGValue(bot) - GetGValue(top)) * q / 1000,
                             GetBValue(top) + (GetBValue(bot) - GetBValue(top)) * q / 1000);
            HBRUSH b = CreateSolidBrush(c);
            RECT rr = { t.left, y, t.right, y + 1 };
            FillRect(dc, &rr, b);
            DeleteObject(b);
        }
    } else {
        HBRUSH br = CreateSolidBrush(p.bg);
        FillRect(dc, &t, br); DeleteObject(br);
    }
    RECT b = { f.left, f.top + th - 1, f.right, f.top + th };
    HBRUSH br2 = CreateSolidBrush(p.line);
    FillRect(dc, &b, br2); DeleteObject(br2);
    SelectObject(dc, g_fontUI);
    SetBkMode(dc, TRANSPARENT);
    std::wstring text = g_model.formProps.count(L"Text") ? g_model.formProps[L"Text"] : L"Form1";
    RECT tr = t;
    if (p.kind == 1) {
        // MAC TRAFİK IŞIKLARI — solda üç renkli daire (kapat/küçült/büyüt)
        int cy = f.top + th / 2;
        const COLORREF lights[3] = { RGB(255,95,87), RGB(254,188,46), RGB(40,200,64) };
        int lx = f.left + 16;
        for (int i = 0; i < 3; i++) {
            HBRUSH lb = CreateSolidBrush(lights[i]);
            HBRUSH lob = (HBRUSH)SelectObject(dc, lb);
            HPEN lp = CreatePen(PS_SOLID, 1, RGB(0,0,0));
            HBRUSH lop = (HBRUSH)SelectObject(dc, lp);
            Ellipse(dc, lx - 5, cy - 5, lx + 5, cy + 5);
            SelectObject(dc, lop); SelectObject(dc, lob);
            DeleteObject(lb); DeleteObject(lp);
            lx += 16;
        }
        tr.left = f.left + 58;   // metin ışıkların sağında
        SetTextColor(dc, p.text);
        DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        return;   // sağda Windows kutucuğu çizilmez (mac)
    }
    SetTextColor(dc, p.text);
    DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (p.kind == 2) {
        // M2God grafit başlık — sağda TEK X düğmesi (ana programdaki btnClose
        // 18×18 tema gradyanlı yuvarlak buton); min/max kutucukları çizilmez
        // (M2God pencereleri FormBorderStyle=None'dır, X'i kendisi çizer)
        RECT r = { f.right - 26, f.top + 4, f.right - 26 + 18, f.top + 4 + 18 };
        // buton gövdesi: 5 duraklı tema gradyanı basitleştirilmiş (üst→alt duraklar)
        int hgt = r.bottom - r.top;
        for (int y = r.top; y < r.bottom; y++) {
            int q = hgt > 0 ? (y - r.top) * 1000 / hgt : 0;
            // üst durak = gradyan üstü, alt durak = gradyan altı (tema başlık renkleri)
            COLORREF top = theme::TitleGradTop(cur), bot = theme::TitleGradBottom(cur);
            COLORREF c = RGB(GetRValue(top) + (GetRValue(bot) - GetRValue(top)) * q / 1000,
                             GetGValue(top) + (GetGValue(bot) - GetGValue(top)) * q / 1000,
                             GetBValue(top) + (GetBValue(bot) - GetBValue(top)) * q / 1000);
            HBRUSH b = CreateSolidBrush(c);
            RECT rr = { r.left, y, r.right, y + 1 };
            FillRect(dc, &rr, b);
            DeleteObject(b);
        }
        HPEN pen = CreatePen(PS_SOLID, 1, p.line);
        HPEN op = (HPEN)SelectObject(dc, pen);
        HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, r.left, r.top, r.right - 1, r.bottom - 1, 10, 10);
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
        SetTextColor(dc, RGB(255,255,255));
        RECT gr = r;
        DrawTextW(dc, L"✕", -1, &gr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    // FAZ 2.8: düğmeler MaximizeBox/MinimizeBox özelliklerini yansıtır
    bool maxB = _wcsicmp(g_model.formProps.count(L"MaximizeBox") ? g_model.formProps[L"MaximizeBox"].c_str() : L"True", L"False") != 0;
    bool minB = _wcsicmp(g_model.formProps.count(L"MinimizeBox") ? g_model.formProps[L"MinimizeBox"].c_str() : L"True", L"False") != 0;
    struct Btn { bool on; const wchar_t* glyp; } bs[3] = {
        { minB, L"_" }, { maxB, L"□" }, { true, L"✕" }
    };
    int bx = f.right - 8;
    for (int i = 2; i >= 0; i--) {   // sağdan sola: kapat, büyüt, küçült
        RECT r = { bx - 22, f.top + 8, bx, f.top + 24 };
        bx -= 22;
        if (!bs[i].on) {   // kapalı düğme soluk çizilir (VS davranışı)
            HBRUSH gb = CreateSolidBrush(p.disabledFill);
            FillRect(dc, &r, gb); DeleteObject(gb);
            HBRUSH lb = CreateSolidBrush(p.disabledLine);
            FrameRect(dc, &r, lb); DeleteObject(lb);
            continue;
        }
        HBRUSH wb = CreateSolidBrush(p.bg);
        FillRect(dc, &r, wb); DeleteObject(wb);
        HBRUSH fb2 = CreateSolidBrush(RGB(128,128,128));
        FrameRect(dc, &r, fb2); DeleteObject(fb2);
        RECT gr = r;
        SetTextColor(dc, p.glyph);
        DrawTextW(dc, bs[i].glyp, -1, &gr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static void DrawGrid(HDC dc, const RECT& client) {
    if (!g_gridVisible) return;
    int gx = GRID_SIZE * 2;
    for (int x = gx - (s_scrollX % gx); x < client.right; x += gx) {
        if (x < client.left) continue;
        for (int y = gx - (s_scrollY % gx); y < client.bottom; y += gx) {
            if (y < client.top) continue;
            SetPixel(dc, x, y, RGB(208, 208, 208));
        }
    }
}

static void DrawComponentIcon(HDC dc, int x, int y, const std::wstring& type) {
    HBRUSH br = CreateSolidBrush(RGB(0, 120, 215));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 90, 158));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (type == L"Timer") {
        Ellipse(dc, x, y, x + 16, y + 16);
        SelectObject(dc, br);
        MoveToEx(dc, x + 8, y + 8, nullptr); LineTo(dc, x + 8, y + 3);
        MoveToEx(dc, x + 8, y + 8, nullptr); LineTo(dc, x + 12, y + 10);
    } else {
        Rectangle(dc, x, y, x + 16, y + 16);
        SelectObject(dc, br);
        MoveToEx(dc, x + 3, y + 8, nullptr); LineTo(dc, x + 13, y + 8);
        MoveToEx(dc, x + 8, y + 3, nullptr); LineTo(dc, x + 8, y + 13);
    }
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(pen); DeleteObject(br);
}

static void DrawTray(HDC dc, HWND surf) {
    RECT tr = TrayRect(surf);
    RECT sep = { tr.left, tr.top, tr.right, tr.top + 1 };
    HBRUSH br = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(dc, &sep, br); DeleteObject(br);
    int x = tr.left + 12;
    SelectObject(dc, g_fontUI);
    SetBkMode(dc, TRANSPARENT);
    for (auto* c : g_model.ctrls) {
        if (!c->nonVisual) continue;
        SIZE sz; GetTextExtentPoint32W(dc, c->name.c_str(), (int)c->name.size(), &sz);
        int w = 16 + 6 + sz.cx + 12;
        RECT item = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 };
        if (g_model.selected == c) {
            HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
            HPEN op = (HPEN)SelectObject(dc, pen);
            HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, item.left, item.top, item.right, item.bottom);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
        }
        DrawComponentIcon(dc, item.left + 6, item.top + (item.bottom - item.top - 16) / 2, c->type);
        RECT tr2 = item; tr2.left += 24;
        SetTextColor(dc, RGB(30, 30, 30));
        DrawTextW(dc, c->name.c_str(), -1, &tr2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        x += w + 8;
    }
    if (x == tr.left + 12) {
        RECT tr3 = { tr.left + 14, tr.top, tr.right, tr.bottom };
        SetTextColor(dc, RGB(140, 140, 140));
        DrawTextW(dc, L"(Bileşen tepsisi — Timer vb. burada görünür)", -1, &tr3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

static void DrawSelection(HWND dcH, HDC dc) {
    Ctrl* sel = g_model.selected;
    HWND surf = dcH;
    RECT r;
    if (!sel) {
        // FAZ 2.8: FORM SEÇİLİ — istemci alanı çevresinde kesikli çerçeve +
        // 8 BOYUTLANDIRMA TUTAMAÇI (form fareyle büyütülüp küçültülür)
        if (!g_model.formSelected) return;
        RECT cr = ClientAreaRect(surf);
        r = { cr.left - SEL_MARGIN, cr.top - SEL_MARGIN, cr.right + SEL_MARGIN, cr.bottom + SEL_MARGIN };
        HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
        HPEN op = (HPEN)SelectObject(dc, pen);
        HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, r.left, r.top, r.right, r.bottom);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
        for (int i = 0; i < 8; i++) {
            RECT hr = HandleRect(r, i);
            HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(dc, &hr, wb); DeleteObject(wb);
            FrameRect(dc, &hr, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
        return;
    }
    if (sel->nonVisual) {
        RECT tr = TrayRect(surf);
        int x = tr.left + 12;
        HDC tdc = GetDC(surf);
        SelectObject(tdc, g_fontUI);
        for (auto* c : g_model.ctrls) {
            if (!c->nonVisual) continue;
            SIZE sz; GetTextExtentPoint32W(tdc, c->name.c_str(), (int)c->name.size(), &sz);
            int w = 16 + 6 + sz.cx + 12;
            if (c == sel) { r = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 }; break; }
            x += w + 8;
        }
        ReleaseDC(surf, tdc);
    } else {
        // FAZ 2.6.2: TabPage seçimi ana pencere (TabControl) çerçevesinde
        // gösterilir — sayfa zaten görüntü alanını doldurur, VS davranışı
        // budur (seçim geri bildirimi TabControl çevresinde belirir).
        // Sayfa HWND'si kendi alanını kapattığı için sayfa dikdörtgenine
        // çizilen kesikli çizgi görünmüyordu.
        Ctrl* drawC = sel;
        if (_wcsicmp(sel->type.c_str(), L"TabPage") == 0 && sel->parent)
            drawC = sel->parent;
        RECT cs = CtrlRectOnSurface(drawC, surf);
        r = { cs.left - SEL_MARGIN, cs.top - SEL_MARGIN, cs.right + SEL_MARGIN, cs.bottom + SEL_MARGIN };
    }
    HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, r.left, r.top, r.right, r.bottom);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
    // FAZ 2.6: TabPage seçiminde boyutlandırma tutamaçları YOK (boyut sekme
    // görüntü alanına bağlıdır — VS davranışı). Sayfa gövdesi tıklanınca
    // seçim zaten TabControl'e yönlenir; tutamaçlar yalnız TabControl'de olur.
    bool noHandles = sel->nonVisual || _wcsicmp(sel->type.c_str(), L"TabPage") == 0;
    if (!noHandles) {
        for (int i = 0; i < 8; i++) {
            RECT hr = HandleRect(r, i);
            HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(dc, &hr, wb); DeleteObject(wb);
            FrameRect(dc, &hr, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
    }
}

static void Paint(HWND surf) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(surf, &ps);
    RECT rc; GetClientRect(surf, &rc);
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
    HBITMAP ob = (HBITMAP)SelectObject(mem, bmp);
    HBRUSH bg = CreateSolidBrush(RGB(229, 229, 229));
    FillRect(mem, &rc, bg); DeleteObject(bg);
    RECT f = FormRect(surf);
    RECT sh = f; OffsetRect(&sh, 4, 4);
    HBRUSH shb = CreateSolidBrush(RGB(205, 205, 205));
    FillRect(mem, &sh, shb); DeleteObject(shb);
    FrameRect(mem, &f, (HBRUSH)GetStockObject(BLACK_BRUSH));
    COLORREF formBack = RGB(255, 255, 255);
    std::wstring fb = g_model.formProps.count(L"BackColor") ? g_model.formProps[L"BackColor"] : L"Control";
    COLORREF t; if (ParseColor(fb, t)) formBack = t;
    // FAZ 2.11: APPLE & MAC — "Control" zemin temanın kendi
    // pencere zemini olur (mac #ECECEC, modern #F0F0F0); diğer temalarda beyaz
    if (_wcsicmp(fb.c_str(), L"Control") == 0)
        formBack = theme::FormBackColorOf(theme::Current());
    RECT client = ClientAreaRect(surf);
    HBRUSH fb2 = CreateSolidBrush(formBack);
    FillRect(mem, &client, fb2); DeleteObject(fb2);
    DrawGrid(mem, client);
    DrawTitleBar(mem, f);
    DrawTray(mem, surf);
    // overlay sürükleme sırasında seçim çerçevesini katman çizer (çift çizgi olmasın)
    if (!(s_drag && s_dragMode == 1 && s_started))
        DrawSelection(surf, mem);
    // FAZ 2.6: seçili TabControl'ün akıllı oku (VS smart tag)
    if (g_model.selected && !g_model.formSelected &&
        _wcsicmp(g_model.selected->type.c_str(), L"TabControl") == 0)
        DrawSmartTag(mem, surf, g_model.selected);
    BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, ob);
    DeleteObject(bmp); DeleteDC(mem);
    EndPaint(surf, &ps);
}

// ===PART2===

// --- etkileşim ----------------------------------------------------------------
static int HitHandle(HWND surf, int sx, int sy) {
    Ctrl* sel = g_model.selected;
    if (!sel || sel->nonVisual) return -1;
    // FAZ 2.6.2: TabPage'in tutamaçları YOK — sayfa boyutu sekme görüntü
    // alanına bağlıdır; seçim çerçevesi TabControl çevresinde çizilir ama
    // tutamaç isabeti sayfa seçiminde devre dışıdır (yanlış boyutlandırma
    // engellenir). Boyutlandırma için gövdeye tıkla → TabControl seçilir.
    if (_wcsicmp(sel->type.c_str(), L"TabPage") == 0) return -1;
    RECT cs = CtrlRectOnSurface(sel, surf);
    RECT r = { cs.left - SEL_MARGIN, cs.top - SEL_MARGIN, cs.right + SEL_MARGIN, cs.bottom + SEL_MARGIN };
    for (int i = 0; i < 8; i++) {
        RECT hr = HandleRect(r, i);
        InflateRect(&hr, 3, 3);
        if (PtInRect(&hr, { sx, sy })) return i;
    }
    return -1;
}

// FAZ 2.8: form seçiliyken istemci alanı kenarındaki 8 tutamacın isabeti.
// Formu fareyle boyutlandırmak için kullanılır (ClientSize güncellenir).
static int HitFormHandle(HWND surf, int sx, int sy) {
    if (!(g_model.formSelected && !g_model.selected)) return -1;
    if (s_formDrag) return -1;
    RECT cr = ClientAreaRect(surf);
    RECT r = { cr.left - SEL_MARGIN, cr.top - SEL_MARGIN, cr.right + SEL_MARGIN, cr.bottom + SEL_MARGIN };
    for (int i = 0; i < 8; i++) {
        RECT hr = HandleRect(r, i);
        InflateRect(&hr, 3, 3);
        if (PtInRect(&hr, { sx, sy })) return i;
    }
    return -1;
}

static Ctrl* HitTray(HWND surf, int sx, int sy) {
    RECT tr = TrayRect(surf);
    if (!PtInRect(&tr, { sx, sy })) return nullptr;
    HDC tdc = GetDC(surf);
    SelectObject(tdc, g_fontUI);
    int x = tr.left + 12;
    for (auto* c : g_model.ctrls) {
        if (!c->nonVisual) continue;
        SIZE sz; GetTextExtentPoint32W(tdc, c->name.c_str(), (int)c->name.size(), &sz);
        int w = 16 + 6 + sz.cx + 12;
        RECT item = { x, tr.top + 8, x + w, tr.top + TRAY_H - 8 };
        if (PtInRect(&item, { sx, sy })) { ReleaseDC(surf, tdc); return c; }
        x += w + 8;
    }
    ReleaseDC(surf, tdc);
    return nullptr;
}

static void NotifySelChanged() {
    PostMessageW(g_hMain, MWM_SEL_CHANGED, (WPARAM)g_model.selected, 0);
}

// eski seçim kenarları/hayalet pikseller kalmasın: tüm çocuklarla tam yeniden çiz
static void RepaintAll(HWND surf) {
    RedrawWindow(surf, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

// aktif sürükleme kontrolünü güvenle iptal et (silme/undo sırasında çağrılır)
// FAZ 2.6.1 — VS davranışı ESC zinciri. Hızlandırıcı tablosu VK_ESCAPE'ı
// CMD_ESC_CANCEL'a taşıdığı için surface'ın kendi WM_KEYDOWN'ı Escape'i
// GÖRMEZ; zincir burada yaşar ve her odaktan çalışır.
bool EscapeChain() {
    // 0) FAZ 2.8: form boyutlandırma sürüklemesini iptal et — başlangıca dön
    if (s_formDrag) {
        s_formX = s_formStartRect.left; s_formY = s_formStartRect.top;
        g_model.formW = s_formStartRect.right - s_formStartRect.left;
        g_model.formH = s_formStartRect.bottom - s_formStartRect.top;
        wchar_t b[64]; swprintf(b, 64, L"%d, %d", g_model.formW, g_model.formH);
        g_model.formProps[L"ClientSize"] = b;
        s_formDrag = false; s_handle = -1; s_started = false;
        LayoutAll(s_surf); RepaintAll(s_surf);
        NotifySelChanged();
        Status(L"Form boyutlandırma iptal edildi");
        return true;
    }
    // 1) aktif sürüklemeyi iptal et: başlangıç konumuna dön
    if (s_drag) {
        Ctrl* d = s_drag;
        DragLayerHide();
        s_drag = nullptr; s_dragMode = 0; s_started = false;
        d->x = s_startRect.left; d->y = s_startRect.top;
        wchar_t b[64]; swprintf(b, 64, L"%d, %d", d->x, d->y);
        d->props[L"Location"] = b;
        if (d->hwnd && IsWindow(d->hwnd)) { PositionOne(s_surf, d); ShowWindow(d->hwnd, SW_SHOWNA); }
        RepaintAll(s_surf);
        NotifySelChanged();
        Status(L"Taşıma iptal edildi");
        return true;
    }
    // 2) tıkla-yerleştir modunu iptal et
    if (!toolbox::ArmedType().empty()) { toolbox::Disarm(); Status(L"Yerleştirme iptal edildi"); RepaintAll(s_surf); return true; }
    // 3) VS davranışı: seçimi ÜST KONTROLE yürüt (sayfa → TabControl, iç → konteyner)
    if (g_model.selected && g_model.selected->parent) {
        Ctrl* up = g_model.selected->parent;
        g_model.selected = up; g_model.formSelected = false;
        NotifySelChanged(); RepaintAll(s_surf);
        Status(up->name + L" seçildi");
        return true;
    }
    // 4) seçimi kaldır (form seçili olur)
    if (g_model.selected) {
        g_model.selected = nullptr; g_model.formSelected = true;
        NotifySelChanged(); RepaintAll(s_surf);
        return true;
    }
    return false;
}

void OnModelDelete(Ctrl* c) {
    if (!s_drag) return;
    if (s_drag == c || g_model.IsAncestorOf(c, s_drag) || g_model.IsAncestorOf(s_drag, c)) {
        DragLayerHide();
        // gerçek HWND gizliydi (overlay sürükleme) — silinmeden önce göster
        if (c->hwnd && IsWindow(c->hwnd)) ShowWindow(c->hwnd, SW_SHOWNA);
        s_drag = nullptr; s_dragMode = 0; s_handle = -1;
    }
}

// araç kutusundan yeni kontrol yerleştir (bırakma VE tıkla-yerleştir ortak yolu)
void PlaceNew(HWND surf, const std::wstring& type, int sx, int sy) {
    const ElementDef* def = g_catalog.Find(type);
    if (!def) return;
    int fx, fy; ScreenToForm(surf, sx, sy, &fx, &fy);
    Ctrl* parent = DropParentAt(surf, sx, sy, nullptr);
    int px = fx, py = fy;
    if (parent) {
        POINT ap = g_model.AbsPos(parent);
        px = fx - ap.x; py = fy - ap.y;
    }
    factory::Win32Class m = factory::MapClass(type);
    int w = m.dw > 0 ? m.dw : 100, h = m.dh > 0 ? m.dh : 30;
    if (s_gridSnap) { px = ((px + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; py = ((py + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
    if (parent) ClampIntoContainer(parent, px, py, w, h);
    else { px = max(0, min(px, g_model.formW - 8)); py = max(0, min(py, g_model.formH - 8)); }
    g_model.PushUndo();
    Ctrl* c = g_model.Add(type, px, py, parent);
    if (!c) return;
    c->w = w; c->h = h;
    wchar_t b1[64], b2[64];
    swprintf(b1, 64, L"%d, %d", c->x, c->y);
    swprintf(b2, 64, L"%d, %d", c->w, c->h);
    c->props[L"Location"] = b1; c->props[L"Size"] = b2;
    factory::CreateOnSurface(surf, c);
    g_model.selected = c; g_model.formSelected = false;
    NotifySelChanged();
    RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    std::wstring where = parent ? (L" → " + parent->name + L" içine") : L"";
    Status(type + L" eklendi: " + c->name + where);
}

// sürükleme/boyutlandırmayı uygula — koordinatlar PARENT görelisi
// TAŞIMA (mode 1): gerçek HWND HİZLETİLMEZ (gizli), hayalet katmanda çizilir.
//   → konteyner kenarına çarpma yok, konteynerden akıcı çıkış var,
//   → grup kutusu içinde hayalet/kendini kopyalamış görüntü olmaz.
// BOYUTLANDIRMA (mode 2): HWND canlı boyutlanır + her hamlede tam temizlik.
static void ApplyDrag(HWND surf, int sx, int sy) {
    if (!s_drag) return;
    s_lastCursor = { sx, sy };
    int dx = sx - s_start.x, dy = sy - s_start.y;
    if (s_dragMode == 1) {
        int nx = s_startRect.left + dx, ny = s_startRect.top + dy;
        if (s_gridSnap) { nx = ((nx + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; ny = ((ny + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
        // KONTAYNER CLAMP'İ YOK — kontrol konteynerin dışına serbestçe taşınabilir;
        // hedef katman bırakma anında imleç konumundan belirlenir (VS davranışı)
        s_drag->x = nx; s_drag->y = ny;
        wchar_t b[64]; swprintf(b, 64, L"%d, %d", nx, ny);
        s_drag->props[L"Location"] = b;
        DragLayerInvalidate();          // hayalet + hedef vurgusu yeniden çiz
        PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
        return;
    }
    // --- boyutlandırma (mode 2) ---
    int nw = s_startRect.right - s_startRect.left, nh = s_startRect.bottom - s_startRect.top;
    int x0 = s_startRect.left, y0 = s_startRect.top;
    switch (s_handle) {
    case 0: x0 += dx; y0 += dy; nw -= dx; nh -= dy; break;
    case 1: y0 += dy; nh -= dy; break;
    case 2: x0 += dx; y0 += dy; nw -= dx; nh -= dy; break;
    case 3: nw += dx; break;
    case 4: nw += dx; nh += dy; break;
    case 5: nh += dy; break;
    case 6: x0 += dx; nw -= dx; nh += dy; break;
    case 7: x0 += dx; nw -= dx; break;
    }
    if (s_gridSnap) { nw = max(GRID_SIZE, ((nw + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE); nh = max(GRID_SIZE, ((nh + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE); }
    nw = max(8, nw); nh = max(8, nh);
    s_drag->x = x0; s_drag->y = y0; s_drag->w = nw; s_drag->h = nh;
    wchar_t b1[64], b2[64];
    swprintf(b1, 64, L"%d, %d", x0, y0); swprintf(b2, 64, L"%d, %d", nw, nh);
    s_drag->props[L"Location"] = b1; s_drag->props[L"Size"] = b2;
    PositionOne(surf, s_drag);
    RepaintAll(surf);   // canlı boyutlandırmada hayalet pikselleri anında temizle
    PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);   // hafif güncelleme (flicker yok)
}

// FAZ 2.8 — FORM BOYUTLANDIRMA sürüklemesi: kenar tutamacından sürükleyince
// formun istemci boyutu (formW/formH = ClientSize) canlı değişir. Sol/üst
// tutamaçlar formu yüzeyde kaydırır (kontroller istemci-görelisi kalır — VS).
static void ApplyFormDrag(HWND surf, int sx, int sy) {
    s_lastCursor = { sx, sy };
    int dx = sx - s_start.x, dy = sy - s_start.y;
    int nw = s_formStartRect.right - s_formStartRect.left;
    int nh = s_formStartRect.bottom - s_formStartRect.top;
    int x0 = s_formStartRect.left, y0 = s_formStartRect.top;
    switch (s_handle) {
    case 0: x0 += dx; y0 += dy; nw -= dx; nh -= dy; break;
    case 1: y0 += dy; nh -= dy; break;
    case 2: y0 += dy; nh -= dy; nw += dx; break;
    case 3: nw += dx; break;
    case 4: nw += dx; nh += dy; break;
    case 5: nh += dy; break;
    case 6: x0 += dx; nw -= dx; nh += dy; break;
    case 7: x0 += dx; nw -= dx; break;
    }
    if (s_gridSnap) {
        nw = max(GRID_SIZE, ((nw + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE);
        nh = max(GRID_SIZE, ((nh + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE);
    }
    nw = max(100, nw); nh = max(100, nh);   // ClientSize ile aynı alt sınır
    s_formX = max(0, x0); s_formY = max(0, y0);
    g_model.formW = nw; g_model.formH = nh;
    wchar_t b[64]; swprintf(b, 64, L"%d, %d", nw, nh);
    g_model.formProps[L"ClientSize"] = b;   // özellik paneliyle aynı kaynak
    LayoutAll(surf);        // çocuklar yeni istemci alanına taşınır
    RepaintAll(surf);
    PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);   // ClientSize satırı canlı güncellenir
}

// sürükleme bitince: imlecin üstündeki konteynere al (VS davranışı)
static void FinishDragReparent(HWND surf, Ctrl* dragged) {
    // gerçek kontrolü geri göster (overlay sürükleme sırasında gizliydi)
    if (dragged->hwnd && IsWindow(dragged->hwnd)) ShowWindow(dragged->hwnd, SW_SHOWNA);
    DragLayerHide();
    if (!dragged) { PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return; }
    Ctrl* newParent = DropParentAt(surf, s_lastCursor.x, s_lastCursor.y, dragged);
    if (newParent != dragged->parent) {
        int fx, fy; ScreenToForm(surf, s_lastCursor.x, s_lastCursor.y, &fx, &fy);
        // kontrolün sol üstünü koru (imleç konumu değil)
        RECT ar = AbsRect(dragged);
        int px = ar.left, py = ar.top;
        if (newParent) {
            POINT ap = g_model.AbsPos(newParent);
            px -= ap.x; py -= ap.y;
            ClampIntoContainer(newParent, px, py, dragged->w, dragged->h);
        } else {
            // forma bırakıldı: form sınırları içine kıstır
            px = max(0, min(px, max(0, g_model.formW - dragged->w)));
            py = max(0, min(py, max(0, g_model.formH - dragged->h)));
        }
        g_model.Reparent(dragged, newParent, px, py);
        NotifySelChanged();   // props panelini tazele (parent-görelisi değerler)
        std::wstring where = newParent ? (newParent->name + L" içine alındı") : L"forma alındı";
        Status(dragged->name + L" " + where);
    }
    // HER DURUMDA: HWND'yi model konumuna getir (overlay sürüklemede HWND
    // hareket etmediği için aynı-katman bırakmalarda da zorunlu)
    PositionOne(surf, dragged);
    RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
}

// seçili kontrolün varsayılan olay adı
static std::wstring EvNameOf(Ctrl* c) {
    return c ? g_model.DefaultEventOf(c) : std::wstring(L"Load");
}

// ==================== FAZ 2.6: TabControl akıllı ok + sekme komutları ====================

// VS smart tag: seçili TabControl'ün sağ üst köşesindeki ok dikdörtgeni (yüzey koord)
static bool SmartTagRect(HWND surf, Ctrl* c, RECT& out) {
    if (!c || _wcsicmp(c->type.c_str(), L"TabControl") != 0) return false;
    RECT cs = CtrlRectOnSurface(c, surf);
    out = { cs.right + 2, cs.top - 17, cs.right + 19, cs.top };
    return true;
}

// akıllı oku çiz (Paint'ten çağrılır)
static void DrawSmartTag(HDC dc, HWND surf, Ctrl* c) {
    RECT t;
    if (!SmartTagRect(surf, c, t)) return;
    HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &t, wb); DeleteObject(wb);
    FrameRect(dc, &t, (HBRUSH)GetStockObject(GRAY_BRUSH));
    // sağa bakan üçgen (VS akıllı etiket oku)
    int cx = (t.left + t.right) / 2, cy = (t.top + t.bottom) / 2;
    POINT tri[3] = { { cx - 3, cy - 4 }, { cx - 3, cy + 4 }, { cx + 4, cy } };
    HBRUSH ab = CreateSolidBrush(RGB(0, 90, 158));
    HPEN op = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
    HBRUSH ob = (HBRUSH)SelectObject(dc, ab);
    SetPolyFillMode(dc, WINDING);
    Polygon(dc, tri, 3);
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(ab);
}

// sayfa ekle (VS "Sekme Ekle")
static void TabCmdAdd(HWND surf, Ctrl* tab) {
    if (!tab) return;
    g_model.PushUndo();
    Ctrl* pg = g_model.AddPage(tab, L"");
    if (!pg) return;
    pg->props[L"Text"] = pg->name;      // VS: başlık = tabPageN
    SyncTabText(tab);
    RecreateAll(surf);
    factory::SelectTabPage(surf, tab, pg);
    g_model.selected = pg; g_model.formSelected = false;
    NotifySelChanged(); RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    Status(pg->name + L" eklendi");
}

// sayfa kaldır (VS "Sekme Kaldır"): seçili sayfa → seçili sekme → son sayfa
static void TabCmdRemove(HWND surf, Ctrl* tab) {
    if (!tab) return;
    auto pages = TabPagesOf(tab);
    if (pages.empty()) return;
    Ctrl* victim = nullptr;
    int victimIdx = (int)pages.size() - 1;
    if (g_model.selected && g_model.selected->parent == tab &&
        _wcsicmp(g_model.selected->type.c_str(), L"TabPage") == 0) {
        victim = g_model.selected;
    }
    if (!victim) {
        std::wstring selName = tab->Get(L"SelectedTab");
        for (int i = 0; i < (int)pages.size(); i++)
            if (_wcsicmp(pages[i]->name.c_str(), selName.c_str()) == 0) { victim = pages[i]; victimIdx = i; break; }
    }
    if (!victim) { victim = pages.back(); victimIdx = (int)pages.size() - 1; }
    g_model.PushUndo();
    g_model.RemovePage(tab, victim);    // victim bu çağrıyla serbest bırakılır
    RecreateAll(surf);
    auto rest = TabPagesOf(tab);
    if (!rest.empty()) {
        int keep = min(victimIdx, (int)rest.size() - 1);
        if (keep < 0) keep = 0;
        factory::SelectTabPage(surf, tab, rest[keep]);
    }
    NotifySelChanged(); RepaintAll(surf);
    PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
    Status(L"Sekme kaldırıldı");
}

// akıllı ok menüsü: Sekme Ekle / Sekme Kaldır / Sekmeleri Düzenle
static void ShowTabSmartMenu(HWND surf, Ctrl* tab, const RECT& tag) {
    HMENU m = CreatePopupMenu();
    auto pages = TabPagesOf(tab);
    AppendMenuW(m, MF_STRING, 9101, L"Sekme Ekle");
    AppendMenuW(m, MF_STRING | (pages.empty() ? MF_GRAYED : 0), 9102, L"Sekme Kaldır");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, 9103, L"Sekmeleri Düzenle...");
    POINT pt = { tag.right, tag.bottom };
    ClientToScreen(surf, &pt);
    SetForegroundWindow(g_hMain);
    int cmd = TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, surf, nullptr);
    DestroyMenu(m);
    switch (cmd) {
    case 9101: TabCmdAdd(surf, tab); break;
    case 9102: TabCmdRemove(surf, tab); break;
    case 9103: PostMessageW(g_hMain, MWM_EDIT_TABPAGES, (WPARAM)tab, 0); break;
    }
}

// sağ tık bağlam menüsü — Sil her koşulda erişilebilir
static void ShowContextMenu(HWND surf, int sx, int sy) {
    Ctrl* c = HitControl(surf, sx, sy);
    if (!c) c = HitTray(surf, sx, sy);
    if (c) {
        g_model.selected = c; g_model.formSelected = false;
        NotifySelChanged(); InvalidateRect(surf, nullptr, FALSE);
    }
    bool hasSel = g_model.selected != nullptr;
    HMENU m = CreatePopupMenu();
    if (hasSel) {
        std::wstring label = EvNameOf(g_model.selected) + L" işleyicisi oluştur";
        AppendMenuW(m, MF_STRING, 9001, label.c_str());
        // FAZ 2.13: sağ tık menüsü teması — 8 seçenek (kalıtsal dahil;
        // M2God grafit 4'lüsü eklendi)
        Ctrl* sc = g_model.selected;
        bool isTabPage = _wcsicmp(sc->type.c_str(), L"TabPage") == 0;
        if (!isTabPage && !sc->nonVisual) {
            std::wstring cur = sc->Get(L"Theme");
            HMENU tm = CreatePopupMenu();
            bool inherited = cur.empty() || _wcsicmp(cur.c_str(), L"(Proje Varsayılanı)") == 0;
            bool old = _wcsicmp(cur.c_str(), L"Oldschool") == 0;
            bool mod = _wcsicmp(cur.c_str(), L"Modern") == 0;
            bool mac = _wcsicmp(cur.c_str(), L"AppleMac") == 0 || _wcsicmp(cur.c_str(), L"Apple & Mac") == 0;
            bool rub = _wcsicmp(cur.c_str(), L"Ruby") == 0 || _wcsicmp(cur.c_str(), L"Ruby Graphite") == 0;
            bool yel = _wcsicmp(cur.c_str(), L"Yellow") == 0 || _wcsicmp(cur.c_str(), L"Yellow Graphite") == 0;
            bool pur = _wcsicmp(cur.c_str(), L"Purple") == 0 || _wcsicmp(cur.c_str(), L"Purple Graphite") == 0;
            bool drk = _wcsicmp(cur.c_str(), L"Dark") == 0;
            AppendMenuW(tm, MF_STRING | (inherited ? MF_CHECKED : 0), 9201, L"(Proje Varsayılanı)");
            AppendMenuW(tm, MF_STRING | (old ? MF_CHECKED : 0), 9202, L"Oldschool (Klasik Win32)");
            AppendMenuW(tm, MF_STRING | (mod ? MF_CHECKED : 0), 9203, L"Modern (.NET 4.8 / WinForm benzeri)");
            AppendMenuW(tm, MF_STRING | (mac ? MF_CHECKED : 0), 9204, L"Apple & Mac (Aqua tarzı)");
            AppendMenuW(tm, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(tm, MF_STRING | (rub ? MF_CHECKED : 0), 9205, L"Ruby Graphite (M2God)");
            AppendMenuW(tm, MF_STRING | (yel ? MF_CHECKED : 0), 9206, L"Yellow Graphite (M2God)");
            AppendMenuW(tm, MF_STRING | (pur ? MF_CHECKED : 0), 9207, L"Purple Graphite (M2God)");
            AppendMenuW(tm, MF_STRING | (drk ? MF_CHECKED : 0), 9208, L"Dark (M2God)");
            AppendMenuW(m, MF_POPUP, (UINT_PTR)tm, L"Tema");
        }
        AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_CUT, L"Kes\tCtrl+X");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_COPY, L"Kopyala\tCtrl+C");
    AppendMenuW(m, MF_STRING, M2CMD_PASTE, L"Yapıştır\tCtrl+V");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_DELETE, L"Sil\tDel");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_FRONT, L"Öne Getir\tCtrl+T");
    AppendMenuW(m, MF_STRING | (hasSel ? 0 : MF_GRAYED), M2CMD_BACK, L"Arkaya Gönder\tCtrl+B");
    POINT pt = { sx, sy };
    ClientToScreen(surf, &pt);
    SetForegroundWindow(g_hMain);
    int cmd = TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, surf, nullptr);
    DestroyMenu(m);
    if (cmd == 9001 && g_model.selected) {
        PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)g_model.selected, (LPARAM)_wcsdup(EvNameOf(g_model.selected).c_str()));
    } else if (cmd >= 9201 && cmd <= 9208 && g_model.selected) {
        // FAZ 2.13: nesne özel tema seçimi — YERİNDE deri değişimi
        // (RecreateAll/aktivasyon bağlamı YOK — çökme güvenliği)
        const wchar_t* val = (cmd == 9201) ? L"(Proje Varsayılanı)"
                          : (cmd == 9202) ? L"Oldschool"
                          : (cmd == 9203) ? L"Modern"
                          : (cmd == 9204) ? L"AppleMac"
                          : (cmd == 9205) ? L"Ruby"
                          : (cmd == 9206) ? L"Yellow"
                          : (cmd == 9207) ? L"Purple" : L"Dark";
        Ctrl* sc = g_model.selected;
        g_model.PushUndo();
        if (cmd == 9201) sc->props.erase(L"Theme");   // kalıtsal → özelliği kaldır
        else             sc->props[L"Theme"] = val;
        // yalnız BU denetimin derisi değişir — pencere imha/yeniden yaratma yok
        if (sc->hwnd && IsWindow(sc->hwnd))
            paintctl::Apply(sc->hwnd, sc->type.c_str(), theme::Effective(sc->Get(L"Theme").c_str()));
        NotifySelChanged(); RepaintAll(surf);
        PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
        Status(std::wstring(sc->name) + L" teması: " + theme::NameOf(theme::Effective(val)));
    } else if (cmd) {
        PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
    }
}

// ===PART3===

LRESULT CALLBACK SurfaceProc(HWND surf, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: Paint(surf); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: UpdateScroll(surf); InvalidateRect(surf, nullptr, FALSE); return 0;
    case WM_HSCROLL: {
        int req = LOWORD(wp);
        if (req == SB_LINERIGHT || req == SB_PAGERIGHT) DoScroll(surf, req == SB_LINERIGHT ? 40 : 160, 0);
        else if (req == SB_LINELEFT || req == SB_PAGELEFT) DoScroll(surf, req == SB_LINELEFT ? -40 : -160, 0);
        else if (req == SB_THUMBTRACK) DoScroll(surf, (int)HIWORD(wp) - s_scrollX, 0);
        return 0;
    }
    case WM_VSCROLL: {
        int req = LOWORD(wp);
        if (req == SB_LINEDOWN || req == SB_PAGEDOWN) DoScroll(surf, 0, req == SB_LINEDOWN ? 40 : 160);
        else if (req == SB_LINEUP || req == SB_PAGEUP) DoScroll(surf, 0, req == SB_LINEUP ? -40 : -160);
        else if (req == SB_THUMBTRACK) DoScroll(surf, 0, (int)HIWORD(wp) - s_scrollY);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -120 : 120;
        if (GetKeyState(VK_SHIFT) & 0x8000) DoScroll(surf, d, 0); else DoScroll(surf, 0, d);
        return 0;
    }
    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT) {
            if (!toolbox::ArmedType().empty()) {
                SetCursor(LoadCursor(nullptr, IDC_CROSS));   // VS: yerleştirme imleci
                return TRUE;
            }
        }
        break;   // alttaki WM_MOUSEMOVE imleç mantığına bırak
    }
    case WM_LBUTTONDOWN: {
        SetFocus(surf);
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        s_lastCursor = pt;
        // 1) araç kutusundan tıkla-yerleştir modu
        std::wstring armed = toolbox::ArmedType();
        if (!armed.empty()) {
            toolbox::Disarm();
            PlaceNew(surf, armed, pt.x, pt.y);
            Status(L"Hazır");
            return 0;
        }
        // 2) FAZ 2.6: akıllı ok (VS smart tag) — TUTAMAÇTAN ÖNCE (ok, köşe
        //    tutamacının üstünde çizilir; VS davranışı: akıllı etiket kazanır)
        if (g_model.selected && _wcsicmp(g_model.selected->type.c_str(), L"TabControl") == 0) {
            RECT tag;
            if (SmartTagRect(surf, g_model.selected, tag) && PtInRect(&tag, pt)) {
                ShowTabSmartMenu(surf, g_model.selected, tag);
                return 0;
            }
        }
        // 3) tutamaç (boyutlandırma)
        int hIdx = HitHandle(surf, pt.x, pt.y);
        if (hIdx >= 0 && g_model.selected) {
            g_model.PushUndo();
            s_drag = g_model.selected; s_dragMode = 2; s_handle = hIdx;
            s_start = pt; s_started = false;
            s_startRect = { s_drag->x, s_drag->y, s_drag->x + s_drag->w, s_drag->y + s_drag->h };
            SetCapture(surf);
            return 0;
        }
        // 3b) FAZ 2.8: FORM KENARI TUTAMAÇLARI — formu fareyle boyutlandır
        // (form seçiliyken istemci alanı kenarında 8 tutamaç görünür)
        int fh = HitFormHandle(surf, pt.x, pt.y);
        if (fh >= 0) {
            g_model.PushUndo();
            s_formDrag = true; s_handle = fh; s_start = pt; s_started = false;
            s_formStartRect = { s_formX, s_formY, s_formX + g_model.formW, s_formY + g_model.formH };
            SetCapture(surf);
            return 0;
        }
        // 2.6) FAZ 2.6: sekme şeridine tıklandı → o sekmeyi (TabPage Ctrl) seç
        Ctrl* hitPre = HitControl(surf, pt.x, pt.y);
        if (hitPre && _wcsicmp(hitPre->type.c_str(), L"TabControl") == 0) {
            int tIdx = factory::TabStripHitIndex(surf, hitPre, pt.x, pt.y);
            if (tIdx >= 0) {
                auto pages = TabPagesOf(hitPre);
                if (tIdx < (int)pages.size()) {
                    Ctrl* pg = pages[tIdx];
                    factory::SelectTabPage(surf, hitPre, pg);
                    g_model.selected = pg; g_model.formSelected = false;
                    NotifySelChanged(); RepaintAll(surf);
                    Status(pg->name + L" seçildi (sayfa)");
                    return 0;
                }
            }
        }
        // FAZ 2.6.2: SAYFA GÖVDESİNE tıklandı/sürüklendi → sayfa kendisi
        // taşINAMAZ ve boyutlandırILAMAZ (VS davranışı + kullanıcı isteği:
        // "tabpagenin iç alanı sürüklenemez olmalı"). Seçim ve sürükleme
        // TÜM TabControl'e gider; böylece tutamaçlar (boyutlandırma) da
        // anında görünür. Sayfa içindeki kontroller (buton vb.) yine
        // kendileri seçilir/sürüklenir — yalnız boş sayfa zemini bu yola girer.
        if (hitPre && hitPre->parent &&
            _wcsicmp(hitPre->type.c_str(), L"TabPage") == 0 &&
            _wcsicmp(hitPre->parent->type.c_str(), L"TabControl") == 0) {
            Ctrl* tab = hitPre->parent;
            if (g_model.selected != tab) {
                g_model.selected = tab; g_model.formSelected = false;
                NotifySelChanged(); RepaintAll(surf);
            }
            g_model.PushUndo();
            s_drag = tab; s_dragMode = 1; s_start = pt; s_started = false;
            s_startRect = { tab->x, tab->y, tab->x + tab->w, tab->y + tab->h };
            SetCapture(surf);
            Status(tab->name + L" seçildi (sayfa gövdesi → tamamı)");
            return 0;
        }
        // 3) kontrol seç + taşı (parent-child duyarlı: üstteki kontrol kazanır)
        Ctrl* c = hitPre;
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (c) {
            if (g_model.selected != c) {
                g_model.selected = c; g_model.formSelected = false;
                NotifySelChanged(); RepaintAll(surf);
            }
            g_model.PushUndo();
            s_drag = c; s_dragMode = 1; s_start = pt; s_started = false;
            s_startRect = { c->x, c->y, c->x + c->w, c->y + c->h };
            SetCapture(surf);
        } else {
            if (!g_model.formSelected) {
                g_model.selected = nullptr; g_model.formSelected = true;
                NotifySelChanged(); RepaintAll(surf);
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        s_lastCursor = pt;
        if ((s_drag || s_formDrag) && (wp & MK_LBUTTON)) {
            if (!s_started) {
                int ddx = abs(pt.x - s_start.x), ddy = abs(pt.y - s_start.y);
                if (ddx > 2 || ddy > 2) {
                    s_started = true;
                    // OVERLAY SÜRÜKLEME başlıyor: gerçek kontrol gizlenir, hayalet katman gösterilir
                    if (s_dragMode == 1 && s_drag && s_drag->hwnd && IsWindow(s_drag->hwnd) && !s_drag->nonVisual)
                        ShowWindow(s_drag->hwnd, SW_HIDE);
                    if (s_dragMode == 1 && s_drag && !s_drag->nonVisual) DragLayerShow(surf);
                    InvalidateRect(surf, nullptr, FALSE);
                }
            }
            if (s_started) {
                if (s_formDrag) ApplyFormDrag(surf, pt.x, pt.y);   // FAZ 2.8: form boyutlandırma
                else ApplyDrag(surf, pt.x, pt.y);
            }
            return 0;
        }
        // imleç
        LPWSTR cur = IDC_ARROW;
        std::wstring armed = toolbox::ArmedType();
        if (armed.empty()) {
            int h = HitHandle(surf, pt.x, pt.y);
            if (h == 0 || h == 4) cur = IDC_SIZENWSE;
            else if (h == 2 || h == 6) cur = IDC_SIZENESW;
            else if (h == 3 || h == 7) cur = IDC_SIZEWE;
            else if (h == 1 || h == 5) cur = IDC_SIZENS;
            else {
                // FAZ 2.8: form kenarı tutamaçları → boyutlandırma imleçleri
                int fh = HitFormHandle(surf, pt.x, pt.y);
                if (fh == 0 || fh == 4) cur = IDC_SIZENWSE;
                else if (fh == 2 || fh == 6) cur = IDC_SIZENESW;
                else if (fh == 3 || fh == 7) cur = IDC_SIZEWE;
                else if (fh == 1 || fh == 5) cur = IDC_SIZENS;
                else if (HitControl(surf, pt.x, pt.y) || HitTray(surf, pt.x, pt.y)) cur = IDC_SIZEALL;
            }
        }
        SetCursor(LoadCursor(nullptr, cur));
        // durum çubuğu
        int fx, fy; ScreenToForm(surf, pt.x, pt.y, &fx, &fy);
        wchar_t b[128];
        if (!armed.empty()) swprintf(b, 128, L"%ls yerleştirmek için tıkla — ESC iptal", armed.c_str());
        else if (g_model.selected && !g_model.formSelected) {
            Ctrl* c = g_model.selected;
            swprintf(b, 128, L"Konum: %d, %d   Boyut: %d × %d   [%ls]", fx, fy, c->w, c->h, c->name.c_str());
        } else swprintf(b, 128, L"Form  %d × %d (ClientSize)   Konum: %d, %d   — kenar tutamaçlarından fareyle boyutlandır",
                        g_model.formW, g_model.formH, fx, fy);
        Status(b);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (s_formDrag) {   // FAZ 2.8: form boyutlandırma bitti
            ReleaseCapture();
            bool started = s_started;
            s_formDrag = false; s_handle = -1; s_started = false;
            if (started) {
                RepaintAll(surf);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
                wchar_t b[96];
                swprintf(b, 96, L"Form boyutlandı: ClientSize %d × %d", g_model.formW, g_model.formH);
                Status(b);
            }
            return 0;
        }
        if (s_drag) {
            Ctrl* dragged = s_drag;          // reparent için yakala
            int mode = s_dragMode;
            bool started = s_started;
            ReleaseCapture();
            s_drag = nullptr; s_dragMode = 0; s_started = false;
            if (mode == 2) {
                RepaintAll(surf);
                PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0);
            } else {
                // hayalet katmanı kapat, gerçek kontrolü geri getir, tam temizlik
                DragLayerHide();
                if (started && dragged->hwnd && IsWindow(dragged->hwnd) && !dragged->nonVisual)
                    ShowWindow(dragged->hwnd, SW_SHOWNA);
                RepaintAll(surf);
                if (started) FinishDragReparent(surf, dragged);
            }
        }
        return 0;
    }
    case WM_CAPTURECHANGED: {
        // sürükleme dışarıdan iptal edildiyse (silme vb.) görünümü toparla
        if (s_formDrag) {   // FAZ 2.8
            s_formDrag = false; s_handle = -1; s_started = false;
            RepaintAll(surf);
        }
        if (s_drag) {
            Ctrl* d = s_drag;
            DragLayerHide();
            if (d->hwnd && IsWindow(d->hwnd) && !d->nonVisual) ShowWindow(d->hwnd, SW_SHOWNA);
            s_drag = nullptr; s_dragMode = 0; s_started = false;
            RepaintAll(surf);
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        Ctrl* c = HitControl(surf, pt.x, pt.y);
        bool isForm = false;
        if (!c) c = HitTray(surf, pt.x, pt.y);
        if (!c) isForm = true;   // boş alan: form çift tıkı (Load)
        if (c || isForm) {
            std::wstring ev = isForm ? std::wstring(L"Load") : g_model.DefaultEventOf(c);
            wchar_t* heap = _wcsdup(ev.c_str());
            PostMessageW(g_hMain, MWM_EVENT_DBLCLICK, (WPARAM)c, (LPARAM)heap);
        }
        return 0;
    }
    case WM_RBUTTONUP:
        return DefWindowProcW(surf, msg, wp, lp);   // DefWindowProc WM_CONTEXTMENU üretir
    case WM_CONTEXTMENU: {
        // imleç yüzeyin üzerinde mi?
        HWND from = (HWND)wp;
        if (from != surf && from != nullptr) break;
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (lp == -1) { pt.x = pt.y = 0; ClientToScreen(surf, &pt); }
        else ScreenToClient(surf, &pt);
        ShowContextMenu(surf, pt.x, pt.y);
        return 0;
    }
    case WM_KEYDOWN: {
        Ctrl* c = g_model.selected;
        bool shift = GetKeyState(VK_SHIFT) & 0x8000;
        bool ctrl = GetKeyState(VK_CONTROL) & 0x8000;
        int step = s_gridSnap ? GRID_SIZE : 1;
        if (wp == VK_ESCAPE) {
            // FAZ 2.6.1: zincir hızlandırıcı üzerinden de çağrılır (CMD_ESC_CANCEL);
            // burada yalnızca odak doğrudan yüzeydeyse çalışır
            EscapeChain();
            return 0;
        }
        if (wp == VK_DELETE) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_DELETE, 0), 0); return 0; }
        if (wp == 'Z' && ctrl) { g_model.Undo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'Y' && ctrl) { g_model.Redo(); RecreateAll(surf); NotifySelChanged(); InvalidateRect(surf, nullptr, TRUE); PostMessageW(g_hMain, MWM_MODEL_CHANGED, 0, 0); return 0; }
        if (wp == 'C' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_COPY, 0), 0); return 0; }
        if (wp == 'X' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_CUT, 0), 0); return 0; }
        if (wp == 'V' && ctrl) { PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_PASTE, 0), 0); return 0; }
        if (!c || c->nonVisual) return 0;
        int dx = 0, dy = 0, dw = 0, dh = 0;
        switch (wp) {
        case VK_LEFT:  if (shift) dw = -step; else dx = -step; break;
        case VK_RIGHT: if (shift) dw = step;  else dx = step;  break;
        case VK_UP:    if (shift) dh = -step; else dy = -step; break;
        case VK_DOWN:  if (shift) dh = step;  else dy = step;  break;
        default: return 0;
        }
        g_model.PushUndo();
        if (s_gridSnap && dx == 0 && dy == 0) { dw = ((dw + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; dh = ((dh + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE; }
        c->x = max(0, c->x + dx); c->y = max(0, c->y + dy);
        c->w = max(8, c->w + dw); c->h = max(8, c->h + dh);
        if (c->parent) {
            int x0, y0, x1, y1;
            ContainerChildArea(c->parent, x0, y0, x1, y1);
            c->x = max(x0, min(c->x, max(x0, x1 - c->w)));
            c->y = max(y0, min(c->y, max(y0, y1 - c->h)));
        }
        PositionOne(surf, c);
        wchar_t b1[64], b2[64];
        swprintf(b1, 64, L"%d, %d", c->x, c->y); swprintf(b2, 64, L"%d, %d", c->w, c->h);
        c->props[L"Location"] = b1; c->props[L"Size"] = b2;
        RepaintAll(surf);   // konteyner içinde ok tuşuyla taşımada da hayalet kalmasın
        PostMessageW(g_hMain, MWM_PROPS_APPLIED, 0, 0);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {   // FAZ 2.13: ListBox da (grafit koyu alan)
        Ctrl* cc = g_model.FindByCid((UINT)GetDlgCtrlID((HWND)lp));
        if (!cc) cc = g_model.FindByHwnd((HWND)lp);
        if (!cc) break;
        HDC dc = (HDC)wp;
        COLORREF fg, bg;
        bool fgSet = ParseColor(cc->Get(L"ForeColor", L"ControlText"), fg);
        bool bgSet = ParseColor(cc->Get(L"BackColor", L"Control"), bg);
        // FAZ 2.13: M2God grafit temalarda "Control" alanlar KENDİ kısayla BOYANIR —
        // CLR_EDIT_BACK (20,20,20) + beyaz metin (Faz2.5 birebir); grafitsel olmayan
        // (statik metin) ise zeminde kalır, yalnız metin beyaza döner
        if (theme::IsGraphite(theme::Current())) {
            bool isField = (_wcsicmp(cc->type.c_str(), L"TextBox") == 0 ||
                            _wcsicmp(cc->type.c_str(), L"MaskedTextBox") == 0 ||
                            _wcsicmp(cc->type.c_str(), L"NumericUpDown") == 0 ||
                            _wcsicmp(cc->type.c_str(), L"ListBox") == 0 ||
                            _wcsicmp(cc->type.c_str(), L"CheckedListBox") == 0);
            if (isField && !bgSet) { bg = RGB(20, 20, 20); bgSet = true; }
            // statik metin (Label/LinkLabel) — zemin EBEBEYNDE değil, EBEVEYN
            // zemininde boyanır (koyu formda kırlangıç kutusu olmasın)
            if (!isField && !bgSet) { bg = paintctl::ParentBack((HWND)lp, theme::Current()); bgSet = true; }
            if (!fgSet) { fg = RGB(255, 255, 255); fgSet = true; }
        }
        if (!fgSet) fg = RGB(0, 0, 0);
        if (!bgSet) bg = GetSysColor(COLOR_BTNFACE);
        SetTextColor(dc, fg);
        SetBkColor(dc, bg);
        SetBkMode(dc, OPAQUE);
        static std::map<HWND, std::pair<COLORREF, HBRUSH>>& brushCache = s_brushCache;
        auto it = brushCache.find((HWND)lp);
        if (it != brushCache.end() && it->second.first == bg) return (LRESULT)it->second.second;
        if (it != brushCache.end()) DeleteObject(it->second.second);
        HBRUSH br = CreateSolidBrush(bg);
        brushCache[(HWND)lp] = { bg, br };
        return (LRESULT)br;
    }
    case WM_DESTROY:
        // FAZ 2.10: IsWindow koruması — konteyner çocukları parent yok edilirken
        // zaten ölmüş olabilir; ölü/stale HWND'ye DestroyWindow çağrılmaz
        for (auto* c : g_model.ctrls)
            if (c->hwnd) { if (IsWindow(c->hwnd)) DestroyWindow(c->hwnd); c->hwnd = nullptr; }
        // fırça önbelleğini temizle (bayat HWND'ler geri dönüşürse yanlış fırça dönmesin)
        for (auto& kv : s_brushCache) if (kv.second.second) DeleteObject(kv.second.second);
        s_brushCache.clear();
        return 0;
    }
    return DefWindowProcW(surf, msg, wp, lp);
}

// ===PART4===

HWND Create(HWND parent) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = SurfaceProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"M2GOD_SURFACE";
        wc.style = CS_DBLCLKS;
        RegisterClassW(&wc);
        reg = true;
    }
    // NOT: WS_CLIPCHILDREN BİLİNÇLİ YOK — grup kutusu gibi şeffaf kontrollerin
    // iç yüzeyi parent boyasıyla temizlensin (hayalet piksellerin kök nedeni)
    HWND h = CreateWindowExW(0, L"M2GOD_SURFACE", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                             0, 0, 0, 0, parent, (HMENU)IDC_SURFACE, g_hInst, nullptr);
    s_surf = h;
    // sürükleme görsel katmanı (bölgeli child — kontrollerin üstünde, şekli dışında şeffaf)
    static bool regL = false;
    if (!regL) {
        WNDCLASSW wc2 = { 0 };
        wc2.lpfnWndProc = DragLayerProc;
        wc2.hInstance = g_hInst;
        wc2.hbrBackground = nullptr;
        wc2.lpszClassName = L"M2GOD_DRAGLAYER";
        RegisterClassW(&wc2);
        regL = true;
    }
    s_dragLayer = CreateWindowExW(WS_EX_TRANSPARENT, L"M2GOD_DRAGLAYER", L"",
                                  WS_CHILD, 0, 0, 0, 0, h, nullptr, g_hInst, nullptr);
    return h;
}

void SubclassForDesign(HWND h) {
    // fare mesajlarını tasarım yüzeyine aktar (HTTRANSPARENT zincirlenir:
    // konteyner çocuğu → konteyner → yüzey)
    // FAZ 2.10: comctl32 SetWindowSubclass YERİNE saf kullanıcı32 alt sınıfama
    // (çökme düzeltmesi — bkz. DesignSubProc açıklaması)
    if (!h || !IsWindow(h)) return;
    WNDPROC cur = (WNDPROC)GetWindowLongPtrW(h, GWLP_WNDPROC);
    if (cur == DesignSubProc) return;
    SetPropW(h, kDesignOldProcProp, (HANDLE)cur);
    SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)DesignSubProc);
}

// kontrolleri parent zincirine göre oluştur (önce konteyner, sonra çocukları)
static void CreateBranch(HWND surf, Ctrl* c) {
    if (c->nonVisual) return;
    factory::CreateOnSurface(surf, c);
    for (auto* k : g_model.ChildrenOf(c)) CreateBranch(surf, k);
}

void LayoutAll(HWND surf) {
    for (auto* c : g_model.ctrls) PositionOne(surf, c);
    UpdateScroll(surf);
}

void PositionOne(HWND surf, Ctrl* c) {
    if (!c || !c->hwnd) return;
    if (c->parent && c->parent->hwnd) {
        // konteyner çocuğu: konteyner client koordinatı
        SetWindowPos(c->hwnd, nullptr, c->x, c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        RECT cr = ClientAreaRect(surf);
        SetWindowPos(c->hwnd, nullptr, cr.left + c->x, cr.top + c->y, c->w, c->h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    // FAZ 2.6: TabControl taşındı/boyutlandı → sayfa pencerelerini senkronla
    if (_wcsicmp(c->type.c_str(), L"TabControl") == 0)
        factory::UpdateTabPages(surf, c);
}

void RecreateAll(HWND surf) {
    for (auto* c : g_model.ctrls) {
        // FAZ 2.9: tema geçişi yüzeyi komple yeniden kurar;HWND'ler zaten yok
        // edilmiş olabilir — IsWindow korumasız DestroyWindow çağrısı yapılmaz
        if (c->hwnd) {
            if (IsWindow(c->hwnd)) DestroyWindow(c->hwnd);
            c->hwnd = nullptr;
        }
    }
    // z-sırası kurallı: önce parent'lar; aynı parent içinde oluşturma sırası (son = üstte)
    for (auto* c : g_model.ChildrenOf(nullptr)) CreateBranch(surf, c);
    LayoutAll(surf);
    RepaintAll(surf);
}

void RefreshSelection(HWND surf) {
    InvalidateRect(surf, nullptr, FALSE);
}

// FAZ 2.8: çocuklar dahil tam yeniden çizim (özellik panelinden çağrılır —
// FormBorderStyle/ClientSize değişince başlık yüksekliği ve geometri değişir)
void FullRedraw(HWND surf) {
    RepaintAll(surf);
}

void NotifySelection() { NotifySelChanged(); }

} // namespace surface
