// paintctl.cpp — FAZ 2.13: TEMA KONTROL ÇİZİM KATMANI gerçeklemesi (bkz. paintctl.h)
//
// Tek pencere prosedürü (SkinProc) çizer:
//   MODERN=1   .NET 4.8 WinForms görünümü
//   MAC=2      Apple & Mac (kapsül Aqua buton, mac check/radio/group/alan)
//   RUBY=3     M2God GRAFİT — Faz2.5 programının 4 teması (kullanıcı isteği
//   YELLOW=4   "m2god un içinde ruby, yellow, purple, dark themeler mevcut —
//   PURPLE=5   button falan herşeyi kapsasın"):
//   DARK=6       - Button: 5 DURAKLI gradyan (0.0/0.15/0.5/0.85/1.0 — Faz2.5
//                 btn_Paint birebir), yuvarlak köşe r=10, 1px tema kenarı,
//                 (+1,+1) alfa gölgeli metin; hover/basılmış gradyanları ayrı
//                 (Ruby hover: FF9999→CC0000 gibi)
//                 - CheckBox/RadioButton: koyu grafit kutu + tema vurgu dolgusu
//                 (Ruby thumb gradyanı), beyaz işaret, beyaz metin
//                 - GroupBox: M2God karakter kutusu görünümü — #252525 yüz,
//                 yuvarlak köşe, başlık çizgi HİZASINDA (#F1F1F1)
//                 - TextBox/ListBox: koyu #141414 alan + 1px tema kenarı
// skin pencere özelliğinde saklanır; paletler global_template.inc'teki CtlPaint
// ile BİREBİR aynıdır (üretilen uygulamada aynı görünüm).
#include "paintctl.h"
#include "theme.h"
#include "model.h"

namespace paintctl {

// --- flavor / kind -----------------------------------------------------------
enum { FL_MODERN = 1, FL_MAC = 2 };                    // theme::MODERN / APPLEMAC ile aynı değer
enum { FL_RUBY = 3, FL_YELLOW = 4, FL_PURPLE = 5, FL_DARK = 6 };   // = theme::RUBY..DARKM (grafit)
enum { K_BUTTON = 0, K_CHECK = 1, K_RADIO = 2, K_GROUP = 3, K_FIELD = 4, K_COMBO = 5 };

struct Skin { int kind; int flavor; };
static const wchar_t* kSkin  = L"M2GOD_SKIN";
static const wchar_t* kOld   = L"M2GOD_SKINOLD";
static const wchar_t* kHover = L"M2GOD_SKINHOVER";
static const wchar_t* kFocus = L"M2GOD_SKINFOCUS";

COLORREF ThemeBack(int t) {
    switch (t) {
    case FL_MAC:    return RGB(236, 236, 236);
    case FL_MODERN: return RGB(240, 240, 240);
    case FL_RUBY: case FL_YELLOW: case FL_PURPLE: case FL_DARK:
                    return RGB(37, 37, 37);      // M2God #252525
    default:        return RGB(255, 255, 255);
    }
}

COLORREF ParentBack(HWND h, int theme) {
    HWND p = GetParent(h);
    int depth = 0;
    while (p && depth++ < 8) {
        Ctrl* c = g_model.FindByHwnd(p);
        if (c) {
            COLORREF t;
            if (ParseColor(c->Get(L"BackColor", L"Control"), t)) return t;
            return ThemeBack(theme);   // "Control" → tema zemini
        }
        p = GetParent(p);
    }
    return ThemeBack(theme);           // yüzey (form zemini) veya bulunamadı
}

// ---------------------------------------------------------------------------
// M2GOD GRAFİT PALETLERİ — Faz2.5 src/theme.h değerleri BİREBİR
// (MakeRubyTheme / MakeYellowTheme / MakePurpleTheme / MakeDarkTheme — btn_Paint
//  5 duraklı gradyanları normal/hover/basılmış + kenar + metin + gölge alfası)
// ---------------------------------------------------------------------------
struct Graphite5 {          // 5 duraklı buton gradyanı (btn_Paint birebir)
    COLORREF c[5];          // pozisyonlar: 0.0, 0.15, 0.5, 0.85, 1.0
    COLORREF border;
    COLORREF text;
    COLORREF textShadow;
    BYTE     shadowA;
};
struct GraphitePal {
    Graphite5 btnN, btnH, btnP;               // buton: normal/hover/basılmış
    COLORREF fieldBack, fieldBorder;          // alan (edit/list): #141414 + tema kenarı
    COLORREF fieldText;                       // alan metni (beyaz)
    COLORREF boxFill, boxBorder;              // checkbox/radio kutu dolgusu/kenarı
    COLORREF accent;                          // vurgu (işaret/radyo dolgusu) — thumb mid
    COLORREF groupBorder, groupTitle;         // groupbox: #6E6E74 kenar + #F1F1F1 başlık
    COLORREF trackBack;                       // groupbox yüzü = #252525
    COLORREF arrowFillTop, arrowFillBottom;    // DarkComboBox ok düğmesi gradyanı
    COLORREF arrowBorder;                     // ok düğmesi kenarı
};

static const GraphitePal& GraphiteOf(int flavor) {
    // RUBY — Faz2.5 MakeRubyTheme (btn_Paint Ruby durakları birebir)
    static GraphitePal ruby = []() {
        GraphitePal p = {};
        p.btnN = { { RGB(0xFF,0x60,0x60), RGB(0xE0,0x00,0x00), RGB(0xCC,0x00,0x00), RGB(0x99,0x00,0x00), RGB(0xE0,0x00,0x00) },
                   RGB(0x5C,0x04,0x04), RGB(245,245,245), RGB(0,0,0), 80 };
        p.btnH = { { RGB(0xFF,0x99,0x99), RGB(0xFF,0x60,0x60), RGB(0xFF,0x00,0x00), RGB(0xE0,0x00,0x00), RGB(0xCC,0x00,0x00) },
                   RGB(0xA0,0x00,0x00), RGB(255,255,255), RGB(0,0,0), 100 };
        p.btnP = { { RGB(0xFF,0x8A,0x80), RGB(0xFF,0x52,0x52), RGB(0xF4,0x43,0x36), RGB(0xD3,0x2F,0x2F), RGB(0xB7,0x1C,0x1C) },
                   RGB(0x7F,0x00,0x00), RGB(255,255,255), RGB(30,0,0), 100 };
        p.fieldBack = RGB(20,20,20);   p.fieldBorder = RGB(60,10,10);      p.fieldText = RGB(255,255,255);
        p.boxFill = RGB(60,10,10);     p.boxBorder = RGB(138,29,49);
        p.accent = RGB(202,41,40);     // Ruby thumb mid
        p.groupBorder = RGB(0x6E,0x6E,0x74); p.groupTitle = RGB(0xF1,0xF1,0xF1); p.trackBack = RGB(37,37,37);
        p.arrowFillTop = RGB(202,41,40);  p.arrowFillBottom = RGB(207,0,0);  p.arrowBorder = RGB(138,29,49);
        return p;
    }();
    // YELLOW — Faz2.5 MakeYellowTheme
    static GraphitePal yellow = []() {
        GraphitePal p = {};
        p.btnN = { { RGB(0xEB,0xDC,0xA2), RGB(0xEA,0xC0,0x46), RGB(0xE0,0x9E,0x00), RGB(0xC7,0x85,0x00), RGB(0xB2,0x77,0x00) },
                   RGB(0x6A,0x3F,0x00), RGB(245,245,245), RGB(40,30,0), 80 };
        p.btnH = { { RGB(0xFF,0xFA,0xCD), RGB(0xFF,0xEE,0x99), RGB(0xFF,0xD7,0x00), RGB(0xF0,0xB4,0x00), RGB(0xE0,0xA0,0x00) },
                   RGB(0xA5,0x7C,0x00), RGB(255,255,255), RGB(40,30,0), 100 };
        p.btnP = { { RGB(0xFC,0xF9,0xC6), RGB(0xFC,0xE2,0x5C), RGB(0xF7,0xB6,0x00), RGB(0xE1,0x9A,0x00), RGB(0xCC,0x88,0x00) },
                   RGB(0x7B,0x4A,0x00), RGB(255,255,255), RGB(40,30,0), 100 };
        p.fieldBack = RGB(20,20,20);   p.fieldBorder = RGB(70,60,10);     p.fieldText = RGB(255,255,255);
        p.boxFill = RGB(70,60,15);     p.boxBorder = RGB(160,130,30);
        p.accent = RGB(220,160,30);    // Yellow thumb mid
        p.groupBorder = RGB(0x6E,0x6E,0x74); p.groupTitle = RGB(0xF1,0xF1,0xF1); p.trackBack = RGB(37,37,37);
        p.arrowFillTop = RGB(229,180,70); p.arrowFillBottom = RGB(180,120,20); p.arrowBorder = RGB(176,145,117);
        return p;
    }();
    // PURPLE — Faz2.5 MakePurpleTheme
    static GraphitePal purple = []() {
        GraphitePal p = {};
        COLORREF s = RGB(40,10,20), m = RGB(25,6,12), e = RGB(15,3,6);
        p.btnN = { { s, RGB(32,8,16), m, RGB(20,4,9), e }, RGB(210,50,255), RGB(245,245,245), RGB(0,0,0), 80 };
        p.btnH = { { s, RGB(32,8,16), m, RGB(20,4,9), e }, RGB(255,50,255), RGB(255,255,255), RGB(0,0,0), 100 };
        p.btnP = { { RGB(35,8,18), RGB(30,6,16), RGB(20,4,10), RGB(10,1,4), RGB(5,0,2) },
                   RGB(200,50,200), RGB(255,255,255), RGB(0,0,0), 120 };
        p.fieldBack = RGB(20,20,20);   p.fieldBorder = RGB(60,20,80);     p.fieldText = RGB(255,255,255);
        p.boxFill = RGB(60,30,80);     p.boxBorder = RGB(120,50,160);
        p.accent = RGB(140,50,180);    // Purple thumb mid
        p.groupBorder = RGB(0x6E,0x6E,0x74); p.groupTitle = RGB(0xF1,0xF1,0xF1); p.trackBack = RGB(37,37,37);
        p.arrowFillTop = RGB(160,70,200); p.arrowFillBottom = RGB(100,35,130); p.arrowBorder = RGB(110,45,150);
        return p;
    }();
    // DARK — Faz2.5 MakeDarkTheme
    static GraphitePal dark = []() {
        GraphitePal p = {};
        p.btnN = { { RGB(0x60,0x60,0x60), RGB(0x48,0x48,0x48), RGB(0x3C,0x3C,0x3C), RGB(0x30,0x30,0x30), RGB(0x48,0x48,0x48) },
                   RGB(0x20,0x20,0x20), RGB(245,245,245), RGB(0,0,0), 80 };
        p.btnH = { { RGB(0x78,0x78,0x78), RGB(0x5C,0x5C,0x5C), RGB(0x4C,0x4C,0x4C), RGB(0x40,0x40,0x40), RGB(0x5C,0x5C,0x5C) },
                   RGB(0x30,0x30,0x30), RGB(255,255,255), RGB(0,0,0), 100 };
        p.btnP = { { RGB(0x50,0x50,0x50), RGB(0x3C,0x3C,0x3C), RGB(0x30,0x30,0x30), RGB(0x26,0x26,0x26), RGB(0x3C,0x3C,0x3C) },
                   RGB(0x18,0x18,0x18), RGB(255,255,255), RGB(0,0,0), 100 };
        p.fieldBack = RGB(20,20,20);   p.fieldBorder = RGB(50,50,50);     p.fieldText = RGB(255,255,255);
        p.boxFill = RGB(50,50,50);     p.boxBorder = RGB(80,80,80);
        p.accent = RGB(120,120,120);   // Dark thumb top-ish
        p.groupBorder = RGB(0x6E,0x6E,0x74); p.groupTitle = RGB(0xF1,0xF1,0xF1); p.trackBack = RGB(37,37,37);
        p.arrowFillTop = RGB(100,100,100); p.arrowFillBottom = RGB(55,55,55); p.arrowBorder = RGB(75,75,75);
        return p;
    }();
    switch (flavor) {
    case FL_RUBY:   return ruby;
    case FL_YELLOW: return yellow;
    case FL_PURPLE: return purple;
    case FL_DARK:   return dark;
    default:        return dark;
    }
}

// --- çizim yardımcıları --------------------------------------------------------
static void VertGradient(HDC dc, RECT r, COLORREF top, COLORREF bot) {
    int hgt = r.bottom - r.top;
    for (int y = r.top; y < r.bottom; y++) {
        int t = hgt > 0 ? (y - r.top) * 1000 / hgt : 0;
        COLORREF c = RGB(GetRValue(top) + (GetRValue(bot) - GetRValue(top)) * t / 1000,
                         GetGValue(top) + (GetGValue(bot) - GetGValue(top)) * t / 1000,
                         GetBValue(top) + (GetBValue(bot) - GetBValue(top)) * t / 1000);
        HBRUSH b = CreateSolidBrush(c);
        RECT rr = { r.left, y, r.right, y + 1 };
        FillRect(dc, &rr, b);
        DeleteObject(b);
    }
}
// 5 duraklı gradyan — durak pozisyonları 0.0/0.15/0.5/0.85/1.0 (btn_Paint birebir)
static void VertGradient5(HDC dc, RECT r, const Graphite5& g) {
    int hgt = r.bottom - r.top;
    if (hgt <= 0) return;
    static const float pos[5] = { 0.0f, 0.15f, 0.5f, 0.85f, 1.0f };
    for (int y = r.top; y < r.bottom; y++) {
        float f = (hgt > 1) ? (float)(y - r.top) / (float)(hgt - 1) : 0.0f;   // birebir: / (H-1)
        int i = 0;
        while (i < 3 && f > pos[i + 1]) i++;
        float span = pos[i + 1] - pos[i];
        float t = (span > 0) ? (f - pos[i]) / span : 0.0f;
        if (t < 0) t = 0; if (t > 1) t = 1;
        COLORREF a = g.c[i], b = g.c[i + 1];
        COLORREF c = RGB(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * (int)(t * 1000) / 1000,
                         GetGValue(a) + (GetGValue(b) - GetGValue(a)) * (int)(t * 1000) / 1000,
                         GetBValue(a) + (GetBValue(b) - GetBValue(a)) * (int)(t * 1000) / 1000);
        HBRUSH br = CreateSolidBrush(c);
        RECT rr = { r.left, y, r.right, y + 1 };
        FillRect(dc, &rr, br);
        DeleteObject(br);
    }
}
static void FillRound(HDC dc, RECT r, int rad, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    HRGN rg = CreateRoundRectRgn(r.left, r.top, r.right + 1, r.bottom + 1, rad * 2, rad * 2);
    FillRgn(dc, rg, b);
    DeleteObject(rg); DeleteObject(b);
}
static void FrameRound(HDC dc, RECT r, int rad, COLORREF c, int wpx = 1) {
    HPEN p = CreatePen(PS_SOLID, wpx, c);
    HPEN op = (HPEN)SelectObject(dc, p);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad * 2, rad * 2);
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p);
}
static void FrameSquare(HDC dc, RECT r, COLORREF c, int wpx = 1) {
    HPEN p = CreatePen(PS_SOLID, wpx, c);
    HPEN op = (HPEN)SelectObject(dc, p);
    HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, r.left, r.top, r.right, r.bottom);
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p);
}
static std::wstring WndText(HWND h) {
    int len = GetWindowTextLengthW(h);
    std::wstring s(len + 1, L'\0');
    if (len > 0) GetWindowTextW(h, &s[0], len + 1);
    s.resize((size_t)max(0, len));
    return s;
}
static void DrawTextC(HWND h, HDC dc, RECT r, const std::wstring& s, COLORREF c, UINT fmt) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c);
    HFONT f = h ? (HFONT)SendMessageW(h, WM_GETFONT, 0, 0) : nullptr;   // kontrolün kendi fontu
    HFONT of = f ? (HFONT)SelectObject(dc, f) : (HFONT)SelectObject(dc, g_fontUI);
    DrawTextW(dc, s.c_str(), -1, &r, fmt | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (f) SelectObject(dc, of);
}
static void DrawCheckMark(HDC dc, int x0, int y0, COLORREF c, int wpx = 2) {
    // onay işareti: (x0+2,y0+6)→(x0+4.5,y0+9)→(x0+9.5,y0+3)
    HPEN p = CreatePen(PS_SOLID, wpx, c);
    HPEN op = (HPEN)SelectObject(dc, p);
    MoveToEx(dc, x0 + 2, y0 + 6, nullptr);
    LineTo(dc, x0 + 5, y0 + 9);
    LineTo(dc, x0 + 10, y0 + 3);
    SelectObject(dc, op); DeleteObject(p);
}
// alfa karışımlı gölge rengi (textShadow + shadowA) — btn_Paint birebir
static COLORREF BlendToBlack(COLORREF c, BYTE alpha) {
    return RGB(GetRValue(c) * (255 - alpha) / 255, GetGValue(c) * (255 - alpha) / 255,
               GetBValue(c) * (255 - alpha) / 255);
}

// --- M2GOD GRAFİT (Ruby/Yellow/Purple/Dark) çizimleri -------------------------
// penceredeki deriden grafit flavor'ı (PaintGraphiteX yardımcıları için)
static int GraphiteFlavorOf(HWND h) {
    Skin* sk = (Skin*)GetPropW(h, kSkin);
    return sk ? sk->flavor : FL_DARK;
}
// Buton: 5 duraklı gradyan + r=10 yuvarlak köşe + 1px kenar + (+1,+1) gölge
static void PaintGraphiteButton(HWND h, HDC dc, RECT rc) {
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool pushed  = (bs & BST_PUSHED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    const GraphitePal& p = GraphiteOf(GraphiteFlavorOf(h));
    const Graphite5& g = !enabled ? p.btnN : pushed ? p.btnP : hover ? p.btnH : p.btnN;
    std::wstring txt = WndText(h);
    // pathRect = (0,0,W-1,H-1) birebir — alt/sağ son satır dışarıda kalır
    RECT body = { rc.left, rc.top, rc.right - 1, rc.bottom - 1 };
    if (body.right <= body.left) body.right = rc.left + 1;
    if (body.bottom <= body.top) body.bottom = rc.top + 1;
    int rad = 10;                                   // btn_Paint RoundedPath radius=10
    if ((body.right - body.left) < 24 || (body.bottom - body.top) < 24) rad = 6;
    int saved = SaveDC(dc);
    HRGN rgn = CreateRoundRectRgn(body.left, body.top, body.right + 1, body.bottom + 1, rad * 2, rad * 2);
    SelectClipRgn(dc, rgn);
    VertGradient5(dc, body, g);
    DeleteObject(rgn);
    RestoreDC(dc, saved);
    FrameRound(dc, body, rad, enabled ? g.border : RGB(70,70,70));
    if (focused && enabled) {                       // odak: iç kesik çerçeve (grafit)
        HPEN pen = CreatePen(PS_DOT, 1, RGB(200,200,200));
        HPEN op = (HPEN)SelectObject(dc, pen);
        HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, body.left + 3, body.top + 3, body.right - 3, body.bottom - 3);
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
    }
    if (!txt.empty()) {
        COLORREF tcol = enabled ? g.text : RGB(150,150,150);
        // metin gölgesi (+1,+1) alfa ile (basılıysa metin (0,+1))
        RECT sr = { body.left + 1, body.top + 1 + (pushed ? 0 : 1), body.right, body.bottom };
        DrawTextC(h, dc, sr, txt, BlendToBlack(g.textShadow, g.shadowA), DT_CENTER | DT_VCENTER);
        RECT tr = { body.left, body.top + (pushed ? 1 : 0), body.right, body.bottom };
        DrawTextC(h, dc, tr, txt, tcol, DT_CENTER | DT_VCENTER);
    }
}

// CheckBox: koyu grafit kutu + tema vurgusu, beyaz işaret + beyaz metin
static void PaintGraphiteCheck(HWND h, HDC dc, RECT rc, COLORREF back) {
    HBRUSH bb = CreateSolidBrush(back);
    FillRect(dc, &rc, bb); DeleteObject(bb);
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool indet   = (bs & BST_INDETERMINATE) != 0;
    bool pushed  = (bs & BST_PUSHED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    const GraphitePal& p = GraphiteOf(GraphiteFlavorOf(h));
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 7;
    RECT box = { bx, by, bx + 14, by + 14 };
    COLORREF fill = (checked || indet) ? (pushed ? p.boxBorder : p.accent) : p.boxFill;
    COLORREF bord = (checked || indet) ? p.boxBorder : (hover ? p.boxBorder : RGB(110,110,116));
    if (!enabled) { fill = RGB(45,45,45); bord = RGB(70,70,74); }
    FillRound(dc, box, 3, fill);
    FrameRound(dc, box, 3, bord);
    if (enabled) {
        if (checked) DrawCheckMark(dc, bx + 1, by + 1, RGB(255,255,255));
        if (indet) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(255,255,255));
            HPEN op = (HPEN)SelectObject(dc, pen);
            MoveToEx(dc, bx + 4, cy, nullptr); LineTo(dc, bx + 10, cy);
            SelectObject(dc, op); DeleteObject(pen);
        }
    }
    if (focused && enabled)
        FrameRound(dc, { box.left - 2, box.top - 2, box.right + 2, box.bottom + 2 }, 4, RGB(200,200,200), 1);
    RECT tr = { box.right + 5, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(255,255,255) : RGB(150,150,150), DT_LEFT | DT_VCENTER);
}

// RadioButton: koyu grafit daire + tema vurgu iç noktası + beyaz metin
static void PaintGraphiteRadio(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    const GraphitePal& p = GraphiteOf(GraphiteFlavorOf(h));
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 7;
    RECT cir = { bx, by, bx + 14, by + 14 };
    HBRUSH fb = CreateSolidBrush(RGB(45,45,45));
    HBRUSH ob = (HBRUSH)SelectObject(dc, fb);
    HPEN bp = CreatePen(PS_SOLID, 1, checked ? p.boxBorder : (hover ? p.boxBorder : RGB(110,110,116)));
    HPEN op = (HPEN)SelectObject(dc, bp);
    Ellipse(dc, cir.left, cir.top, cir.right, cir.bottom);
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(bp); DeleteObject(fb);
    if (checked && enabled) {           // iç nokta: tema vurgusu
        HBRUSH wb = CreateSolidBrush(p.accent);
        HBRUSH wo = (HBRUSH)SelectObject(dc, wb);
        HPEN np = CreatePen(PS_SOLID, 1, p.accent);
        HPEN no = (HPEN)SelectObject(dc, np);
        Ellipse(dc, bx + 4, by + 4, bx + 10, by + 10);
        SelectObject(dc, no); SelectObject(dc, wo);
        DeleteObject(np); DeleteObject(wb);
    }
    if (focused && enabled) {
        HPEN fp = CreatePen(PS_DOT, 1, RGB(200,200,200));
        HPEN fo = (HPEN)SelectObject(dc, fp);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Ellipse(dc, cir.left - 2, cir.top - 2, cir.right + 2, cir.bottom + 2);
        SelectObject(dc, fo); DeleteObject(fp);
    }
    RECT tr = { cir.right + 5, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(255,255,255) : RGB(150,150,150), DT_LEFT | DT_VCENTER);
}

// GroupBox: M2God karakter kutusu görünümü (PaintCharGroupBox rev4 uyarlaması):
// #252525 yüz + 1px #6E6E74 yuvarlak köşeli çerçeve + çizgi HİZASINDA açık başlık
static void PaintGraphiteGroup(HWND h, HDC dc, RECT rc, COLORREF back) {
    HBRUSH bb = CreateSolidBrush(back);
    FillRect(dc, &rc, bb); DeleteObject(bb);
    const GraphitePal& p = GraphiteOf(GraphiteFlavorOf(h));
    std::wstring txt = WndText(h);
    bool enabled = IsWindowEnabled(h) != FALSE;
    // gövde: üst kenar 8px sonra başlar (RAD başlık derinliği), köşe r=3→6 (editör ölçeği)
    RECT br = { rc.left, rc.top + 8, rc.right - 1, rc.bottom - 1 };
    FrameRound(dc, br, 6, p.groupBorder);
    if (!txt.empty()) {
        SIZE sz; HDC tdc = GetDC(h);
        HFONT mf = (HFONT)SendMessageW(h, WM_GETFONT, 0, 0);
        HFONT mold = (HFONT)SelectObject(tdc, mf ? mf : (HFONT)g_fontUI);
        GetTextExtentPoint32W(tdc, txt.c_str(), (int)txt.size(), &sz);
        SelectObject(tdc, mold);
        ReleaseDC(h, tdc);
        // başlık ÇİZGİ HİZASINDA (RAD gap tekniği): çizgiyi yüz rengiyle kes
        RECT gap = { rc.left + 9, rc.top + 6, rc.left + 9 + sz.cx + 6, rc.top + 11 };
        HBRUSH b = CreateSolidBrush(back);
        FillRect(dc, &gap, b); DeleteObject(b);
        RECT tr = { gap.left + 2, rc.top, gap.right, rc.top + 16 };
        DrawTextC(h, dc, tr, txt, enabled ? p.groupTitle : RGB(150,150,150), DT_LEFT | DT_VCENTER);
    }
}

// ComboBox: M2God DarkComboBox görünümü — koyu gövde + beyaz seçim metni +
// sağda tema renkli ok düğmesi (Faz2.5 comboArrow gradyanı birebir).
// Açılır liste (ComboLBox) sistem penceresidir — kalır (önizleme kapalı
// görünümü temalı olur; tasarım yüzeyinde dropdown açılmaz).
static void PaintGraphiteCombo(HWND h, HDC dc, RECT rc) {
    const GraphitePal& p = GraphiteOf(GraphiteFlavorOf(h));
    bool enabled = IsWindowEnabled(h) != FALSE;
    // gövde: koyu alan
    HBRUSH bb = CreateSolidBrush(enabled ? p.fieldBack : RGB(40,40,40));
    RECT body = { rc.left, rc.top, rc.right - 1, rc.bottom - 1 };
    FillRect(dc, &body, bb); DeleteObject(bb);
    FrameSquare(dc, body, enabled ? p.fieldBorder : RGB(70,70,70), 1);
    // ok düğmesi: sağda 17px, tema gradyanı + kenar
    int aw = 17;
    RECT ar = { rc.right - 1 - aw, rc.top, rc.right - 1, rc.bottom - 1 };
    int saved = SaveDC(dc);
    HRGN rg = CreateRectRgn(ar.left, ar.top, ar.right, ar.bottom);
    SelectClipRgn(dc, rg);
    VertGradient(dc, ar, p.arrowFillTop, p.arrowFillBottom);
    SelectClipRgn(dc, nullptr);
    DeleteObject(rg);
    RestoreDC(dc, saved);
    FrameSquare(dc, { ar.left, ar.top, ar.right, ar.bottom }, p.arrowBorder, 1);
    // beyaz ok işareti (küçük üçgen)
    int cx = (ar.left + ar.right) / 2, cy = (ar.top + ar.bottom) / 2;
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH fb = CreateSolidBrush(RGB(255,255,255));
    HBRUSH ob = (HBRUSH)SelectObject(dc, fb);
    POINT tri[3] = { { cx - 4, cy - 2 }, { cx + 4, cy - 2 }, { cx, cy + 3 } };
    Polygon(dc, tri, 3);
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(pen); DeleteObject(fb);
    // seçili metin beyaz
    std::wstring txt = WndText(h);
    if (!txt.empty()) {
        RECT tr = { rc.left + 5, rc.top, rc.right - aw - 4, rc.bottom - 1 };
        DrawTextC(h, dc, tr, txt, enabled ? RGB(255,255,255) : RGB(150,150,150), DT_LEFT | DT_VCENTER);
    }
}

// --- MAC (AQUA) çizimleri ---------------------------------------------------------
// FAZ 2.12: "aşırı az benziyor, buton falan değişmemiş" şikayeti — KAPSÜL
// buton (tam yuvarlak uçlar + parlak üst bant), Big Sur tarzı düz mavi
// checkbox/radio, NSBox grup, odakta mavi çerçeveli TextField/List.
static void PaintMacButton(HWND h, HDC dc, RECT rc) {
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool pushed  = (bs & BST_PUSHED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    bool defBtn  = ((GetWindowLongPtrW(h, GWL_STYLE) & BS_TYPEMASK) == BS_DEFPUSHBUTTON);
    std::wstring txt = WndText(h);
    int cx = rc.right - rc.left, cy = rc.bottom - rc.top;
    int rad = max(4, min(min(cx, cy) / 2, 16));   // KAPSÜL: uçlar tam yuvarlak
    COLORREF top, bot, bord, tcol, gloss;
    if (defBtn) {                       // macOS default: sistem mavisi kapsül
        top   = pushed ? RGB(28, 100, 205) : RGB(79, 163, 255);
        bot   = pushed ? RGB(16, 80, 170)  : RGB(10, 100, 210);
        bord  = pushed ? RGB(10, 70, 150)  : RGB(8, 84, 174);
        tcol  = RGB(255, 255, 255);
        gloss = pushed ? RGB(120, 170, 235) : RGB(190, 222, 255);
    } else if (pushed) {
        top = RGB(220, 220, 224); bot = RGB(200, 200, 205);
        bord = RGB(138, 138, 144); tcol = RGB(29, 29, 31); gloss = 0;
    } else {
        top = RGB(255, 255, 255); bot = RGB(227, 227, 230);
        bord = hover ? RGB(168, 168, 174) : RGB(200, 200, 205);
        tcol = RGB(29, 29, 31); gloss = RGB(255, 255, 255);
    }
    if (!enabled) { top = RGB(240, 240, 242); bot = RGB(233, 233, 235);
                   tcol = RGB(176, 176, 182); bord = RGB(220, 220, 224); gloss = 0; }
    int saved = SaveDC(dc);
    HRGN rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, rad * 2, rad * 2);
    SelectClipRgn(dc, rgn);
    VertGradient(dc, rc, top, bot);
    if (gloss) {                        // parlak üst bant (Aqua hissi)
        int gh = max(2, cy / 4);
        RECT gl = { rc.left + 2, rc.top + 1, rc.right - 2, rc.top + 1 + gh };
        HBRUSH gb = CreateSolidBrush(gloss);
        FillRect(dc, &gl, gb); DeleteObject(gb);
        gl = { rc.left + 2, rc.top + 1 + gh, rc.right - 2, rc.top + 2 + gh };  // bant altı yumuşak geçiş
        HBRUSH gb2 = CreateSolidBrush(RGB(
            (GetRValue(gloss) + GetRValue(bot)) / 2,
            (GetGValue(gloss) + GetBValue(bot)) / 2,
            (GetBValue(gloss) + GetBValue(bot)) / 2));
        FillRect(dc, &gl, gb2); DeleteObject(gb2);
    }
    DeleteObject(rgn);
    RestoreDC(dc, saved);
    FrameRound(dc, rc, rad, bord);
    if (focused && enabled)             // odak halkası (mac mavi)
        FrameRound(dc, { rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2 },
                   max(2, rad - 2), RGB(10, 122, 255), 2);
    DrawTextC(h, dc, rc, txt, tcol, DT_CENTER | DT_VCENTER);
}
static void PaintMacCheck(HWND h, HDC dc, RECT rc, COLORREF back) {
    HBRUSH bb = CreateSolidBrush(back);
    FillRect(dc, &rc, bb); DeleteObject(bb);
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool indet   = (bs & BST_INDETERMINATE) != 0;
    bool pushed  = (bs & BST_PUSHED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 8;                  // 16x16 kutu (mac ölçüsü)
    RECT box = { bx, by, bx + 16, by + 16 };
    // Big Sur tarzı: DÜZ sistem mavisi dolgu, beyaz kalın işaret
    COLORREF fill = checked || indet ? (pushed ? RGB(8, 104, 216) : RGB(10, 122, 255)) : RGB(255, 255, 255);
    COLORREF bord = (checked || indet) ? RGB(8, 104, 216) : (hover ? RGB(138, 138, 144) : RGB(213, 213, 216));
    if (!enabled) { fill = RGB(240, 240, 242); bord = RGB(221, 221, 224); }
    FillRound(dc, box, 4, fill);
    FrameRound(dc, box, 4, bord);
    if (enabled) {
        if (checked) DrawCheckMark(dc, bx + 2, by + 2, RGB(255, 255, 255), 3);
        if (indet) {
            HPEN p = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
            HPEN op = (HPEN)SelectObject(dc, p);
            MoveToEx(dc, bx + 5, cy, nullptr); LineTo(dc, bx + 11, cy);
            SelectObject(dc, op); DeleteObject(p);
        }
    }
    if (focused && enabled)
        FrameRound(dc, { box.left - 2, box.top - 2, box.right + 2, box.bottom + 2 }, 5, RGB(10, 122, 255), 2);
    RECT tr = { box.right + 6, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(29, 29, 31) : RGB(176, 176, 182), DT_LEFT | DT_VCENTER);
}
static void PaintMacRadio(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 8;
    RECT cir = { bx, by, bx + 16, by + 16 };
    HBRUSH fb = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH ob = (HBRUSH)SelectObject(dc, fb);
    HPEN bp = CreatePen(PS_SOLID, 1, checked ? RGB(10, 122, 255)
                         : (hover ? RGB(138, 138, 144) : RGB(213, 213, 216)));
    HPEN op = (HPEN)SelectObject(dc, bp);
    Ellipse(dc, cir.left, cir.top, cir.right, cir.bottom);
    SelectObject(dc, op); SelectObject(dc, ob);
    DeleteObject(bp); DeleteObject(fb);
    if (checked && enabled) {           // iç nokta: sistem mavisi
        HBRUSH wb = CreateSolidBrush(RGB(10, 122, 255));
        HBRUSH wo = (HBRUSH)SelectObject(dc, wb);
        HPEN np = CreatePen(PS_SOLID, 1, RGB(10, 122, 255));
        HPEN no = (HPEN)SelectObject(dc, np);
        Ellipse(dc, bx + 5, by + 5, bx + 11, by + 11);
        SelectObject(dc, no); SelectObject(dc, wo);
        DeleteObject(np); DeleteObject(wb);
    }
    if (focused && enabled) {
        HPEN fp = CreatePen(PS_SOLID, 2, RGB(10, 122, 255));
        HPEN fo = (HPEN)SelectObject(dc, fp);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Ellipse(dc, cir.left - 2, cir.top - 2, cir.right + 2, cir.bottom + 2);
        SelectObject(dc, fo); DeleteObject(fp);
    }
    RECT tr = { cir.right + 6, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(29, 29, 31) : RGB(176, 176, 182), DT_LEFT | DT_VCENTER);
}
static void PaintMacGroup(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    std::wstring txt = WndText(h);
    bool enabled = IsWindowEnabled(h) != FALSE;
    RECT br = { rc.left, rc.top + 8, rc.right - 1, rc.bottom - 1 };
    FrameRound(dc, br, 6, RGB(213, 213, 216));          // NSBox ince yuvarlak çerçeve
    if (!txt.empty()) {
        SIZE sz; HDC tdc = GetDC(h);
        HFONT mf = (HFONT)SendMessageW(h, WM_GETFONT, 0, 0);
        HFONT mold = (HFONT)SelectObject(tdc, mf ? mf : (HFONT)g_fontUI);
        GetTextExtentPoint32W(tdc, txt.c_str(), (int)txt.size(), &sz);
        SelectObject(tdc, mold);
        ReleaseDC(h, tdc);
        RECT tr = { rc.left + 9, rc.top, rc.left + 9 + sz.cx + 6, rc.top + 17 };
        HBRUSH b = CreateSolidBrush(back);
        FillRect(dc, &tr, b); DeleteObject(b);
        DrawTextC(h, dc, { tr.left + 2, tr.top, tr.right, tr.bottom }, txt,
                  enabled ? RGB(60, 60, 67) : RGB(176, 176, 182), DT_LEFT | DT_TOP);
    }
}

// --- MODERN (.NET 4.8 WinForms) çizimleri ----------------------------------------
static void PaintModernButton(HWND h, HDC dc, RECT rc) {
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool pushed  = (bs & BST_PUSHED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    bool defBtn  = ((GetWindowLongPtrW(h, GWL_STYLE) & BS_TYPEMASK) == BS_DEFPUSHBUTTON);
    std::wstring txt = WndText(h);
    COLORREF top, bot, bord, tcol = RGB(0, 0, 0);
    if (!enabled) {
        top = RGB(240, 240, 240); bot = RGB(240, 240, 240);
        bord = RGB(191, 191, 191); tcol = RGB(130, 130, 130);
    } else if (pushed) {
        top = RGB(204, 228, 247); bot = RGB(182, 217, 242); bord = RGB(0, 92, 153);
    } else if (hover) {
        top = RGB(229, 241, 251); bot = RGB(215, 235, 248); bord = RGB(0, 120, 215);
    } else {
        top = RGB(255, 255, 255); bot = RGB(240, 240, 242); bord = RGB(173, 173, 173);
    }
    if (defBtn && enabled) bord = RGB(0, 120, 215);     // WinForms default mavi çerçeve
    int saved = SaveDC(dc);
    HRGN rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, 4, 4);
    SelectClipRgn(dc, rgn);
    VertGradient(dc, rc, top, bot);
    DeleteObject(rgn);
    RestoreDC(dc, saved);
    FrameRound(dc, rc, 2, bord);
    if (focused && enabled) {                          // WinForms odak işareti: iç noktalı çerçeve
        HPEN p = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
        HPEN op = (HPEN)SelectObject(dc, p);
        HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, rc.left + 3, rc.top + 3, rc.right - 3, rc.bottom - 3);
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p);
    }
    DrawTextC(h, dc, rc, txt, tcol, DT_CENTER | DT_VCENTER);
}
static void PaintModernCheck(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool indet   = (bs & BST_INDETERMINATE) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 7;
    RECT box = { bx, by, bx + 14, by + 14 };
    COLORREF fill = (checked || indet) ? RGB(0, 120, 215) : RGB(255, 255, 255);
    COLORREF bord = (checked || indet) ? RGB(0, 90, 158) : (hover ? RGB(0, 120, 215) : RGB(90, 90, 90));
    if (!enabled) { fill = RGB(240, 240, 240); bord = RGB(191, 191, 191); }
    FillRound(dc, box, 2, fill);
    FrameRound(dc, box, 2, bord);
    if (enabled && checked) DrawCheckMark(dc, bx + 1, by + 1, RGB(255, 255, 255));
    if (enabled && indet) {
        HPEN p = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HPEN op = (HPEN)SelectObject(dc, p);
        MoveToEx(dc, bx + 4, cy, nullptr); LineTo(dc, bx + 10, cy);
        SelectObject(dc, op); DeleteObject(p);
    }
    if (focused && enabled)
        FrameRound(dc, { box.left - 2, box.top - 2, box.right + 2, box.bottom + 2 }, 3, RGB(120, 120, 120), 1);
    RECT tr = { box.right + 5, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(0, 0, 0) : RGB(130, 130, 130), DT_LEFT | DT_VCENTER);
}
static void PaintModernRadio(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    int bs = (int)SendMessageW(h, BM_GETSTATE, 0, 0);
    bool checked = (bs & BST_CHECKED) != 0;
    bool focused = (bs & BST_FOCUS) != 0;
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool hover   = GetPropW(h, kHover) != nullptr;
    std::wstring txt = WndText(h);
    int cy = (rc.top + rc.bottom) / 2;
    int bx = rc.left + 2, by = cy - 7;
    RECT cir = { bx, by, bx + 14, by + 14 };
    HBRUSH fb = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH fo = (HBRUSH)SelectObject(dc, fb);
    HPEN bp = CreatePen(PS_SOLID, checked ? 2 : 1, checked ? RGB(0, 90, 158)
                         : (hover ? RGB(0, 120, 215) : RGB(90, 90, 90)));
    HPEN bo = (HPEN)SelectObject(dc, bp);
    Ellipse(dc, cir.left, cir.top, cir.right, cir.bottom);
    SelectObject(dc, bo); SelectObject(dc, fo);
    DeleteObject(bp); DeleteObject(fb);
    if (checked && enabled) {
        HBRUSH db = CreateSolidBrush(RGB(0, 120, 215));
        HBRUSH dobj = (HBRUSH)SelectObject(dc, db);
        HPEN np = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
        HPEN no = (HPEN)SelectObject(dc, np);
        Ellipse(dc, bx + 4, by + 4, bx + 10, by + 10);
        SelectObject(dc, no); SelectObject(dc, dobj);
        DeleteObject(np); DeleteObject(db);
    }
    if (focused && enabled) {
        HPEN fp = CreatePen(PS_DOT, 1, RGB(120, 120, 120));
        HPEN ffo = (HPEN)SelectObject(dc, fp);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Ellipse(dc, cir.left - 2, cir.top - 2, cir.right + 2, cir.bottom + 2);
        SelectObject(dc, ffo); DeleteObject(fp);
    }
    RECT tr = { cir.right + 5, rc.top, rc.right - 2, rc.bottom };
    DrawTextC(h, dc, tr, txt, enabled ? RGB(0, 0, 0) : RGB(130, 130, 130), DT_LEFT | DT_VCENTER);
}
static void PaintModernGroup(HWND h, HDC dc, RECT rc, COLORREF back) {
    FillRect(dc, &rc, CreateSolidBrush(back));
    std::wstring txt = WndText(h);
    bool enabled = IsWindowEnabled(h) != FALSE;
    RECT br = { rc.left, rc.top + 8, rc.right - 1, rc.bottom - 1 };
    FrameSquare(dc, br, RGB(173, 173, 173));           // WinForms GroupBox: düz köşe
    if (!txt.empty()) {
        SIZE sz; HDC tdc = GetDC(h);
        HFONT mf = (HFONT)SendMessageW(h, WM_GETFONT, 0, 0);
        HFONT mold = (HFONT)SelectObject(tdc, mf ? mf : (HFONT)g_fontUI);
        GetTextExtentPoint32W(tdc, txt.c_str(), (int)txt.size(), &sz);
        SelectObject(tdc, mold);
        ReleaseDC(h, tdc);
        RECT tr = { rc.left + 9, rc.top, rc.left + 9 + sz.cx + 6, rc.top + 17 };
        HBRUSH b = CreateSolidBrush(back);
        FillRect(dc, &tr, b); DeleteObject(b);
        DrawTextC(h, dc, { tr.left + 2, tr.top, tr.right, tr.bottom }, txt,
                  enabled ? RGB(0, 0, 0) : RGB(130, 130, 130), DT_LEFT | DT_TOP);
    }
}

// --- FIELD (TextBox / ListBox) kenar çizimi — mac / modern / grafit --------------
static void PaintFieldBorder(HWND h, Skin* sk) {
    // kenar çizgisini NON-CLIENT olarak çiz: istemci alanı (metin) kullanıcı32
    // tarafından boyanmaya devam eder; biz 1px çerçeveyi GetWindowDC ile sararız
    HDC dc = GetWindowDC(h);
    if (!dc) return;
    RECT rc; GetWindowRect(h, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);
    bool enabled = IsWindowEnabled(h) != FALSE;
    bool focused = GetPropW(h, kFocus) != nullptr;
    if (sk->flavor == FL_MAC) {
        COLORREF bord = focused ? RGB(10, 122, 255) : (enabled ? RGB(213, 213, 216) : RGB(225, 225, 228));
        if (focused) FrameRound(dc, rc, 4, bord, 2);    // odakta 2px mac mavisi
        else        FrameRound(dc, rc, 4, bord, 1);
    } else if (theme::IsGraphite(sk->flavor)) {
        // M2God grafit: 1px tema kenarı (Ruby: (60,10,10) v.b.); içi koyu
        // #141414 alan WM_CTLCOLOR ile boyanır (surface tarafı)
        const GraphitePal& p = GraphiteOf(sk->flavor);
        COLORREF bord = focused ? p.accent : (enabled ? p.fieldBorder : RGB(70, 70, 70));
        FrameSquare(dc, rc, bord, 1);
    } else {
        COLORREF bord = focused ? RGB(0, 120, 215) : (enabled ? RGB(173, 173, 173) : RGB(191, 191, 191));
        FrameSquare(dc, rc, bord, 1);                   // WinForms düz tek çizgi
    }
    ReleaseDC(h, dc);
}

// --- ortak prosedür ---------------------------------------------------------------
static void PaintSkin(HWND h, Skin* sk) {
    if (sk->kind == K_FIELD) return;                   // alan: kenar NC'de çizilir
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    RECT rc; GetClientRect(h, &rc);
    COLORREF back = ParentBack(h, sk->flavor);
    switch (sk->kind * 10 + sk->flavor) {
    case K_BUTTON * 10 + FL_MAC:    PaintMacButton(h, dc, rc); break;
    case K_CHECK  * 10 + FL_MAC:    PaintMacCheck(h, dc, rc, back); break;
    case K_RADIO  * 10 + FL_MAC:    PaintMacRadio(h, dc, rc, back); break;
    case K_GROUP  * 10 + FL_MAC:    PaintMacGroup(h, dc, rc, back); break;
    case K_BUTTON * 10 + FL_MODERN: PaintModernButton(h, dc, rc); break;
    case K_CHECK  * 10 + FL_MODERN: PaintModernCheck(h, dc, rc, back); break;
    case K_RADIO  * 10 + FL_MODERN: PaintModernRadio(h, dc, rc, back); break;
    case K_GROUP  * 10 + FL_MODERN: PaintModernGroup(h, dc, rc, back); break;
    default:
        // M2GOD GRAFİT (3..6) — flavor = tema kimliğiyle aynı
        if (theme::IsGraphite(sk->flavor)) {
            switch (sk->kind) {
            case K_BUTTON: PaintGraphiteButton(h, dc, rc); break;
            case K_CHECK:  PaintGraphiteCheck(h, dc, rc, back); break;
            case K_RADIO:  PaintGraphiteRadio(h, dc, rc, back); break;
            case K_GROUP: PaintGraphiteGroup(h, dc, rc, back); break;
            case K_COMBO: PaintGraphiteCombo(h, dc, rc); break;   // DarkComboBox görünümü
            }
        }
        break;
    }
    EndPaint(h, &ps);
}

static LRESULT CALLBACK SkinProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    Skin* sk = (Skin*)GetPropW(h, kSkin);
    // kOld deri söküldükten sonra DAHİ kalır (passthrough zinciri bozulmasın);
    // yalnız gerçek WM_DESTROY'da temizlenir.
    WNDPROC old = (WNDPROC)GetPropW(h, kOld);
    switch (m) {
    case WM_ERASEBKGND:
        if (sk && sk->kind != K_FIELD &&
            !(sk->kind == K_COMBO && !theme::IsGraphite(sk->flavor)))   // combo: grafit değilse standart
            return 1;       // tamamen kendimiz boyuyoruz
        break;
    case WM_PAINT:
        // combo YALNIZ grafit temada özel çizilir; modern/mac standart görünüm
        if (sk && (sk->kind != K_COMBO || theme::IsGraphite(sk->flavor)))
            { PaintSkin(h, sk); return 0; }
        break;
    case WM_NCPAINT:
        // alan derisi: eski NC çizimini yaptır, KENDİ çerçevemizi üstüne çiz
        if (sk && sk->kind == K_FIELD) {
            if (old) CallWindowProcW(old, h, m, w, l);
            PaintFieldBorder(h, sk);
            return 0;
        }
        break;   // diğer türler: varsayılan NC çizimi eski proc'ta
    case WM_MOUSEMOVE:
        if (!GetPropW(h, kHover)) {
            SetPropW(h, kHover, (HANDLE)1);
            TRACKMOUSEEVENT tm = { sizeof(tm), TME_LEAVE, h, 0 };
            TrackMouseEvent(&tm);
            InvalidateRect(h, nullptr, TRUE);
        }
        break;   // eski proc'a ilet (basış durumu vs. işlensin)
    case WM_MOUSELEAVE:
        RemovePropW(h, kHover);
        InvalidateRect(h, nullptr, TRUE);
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (sk && sk->kind == K_FIELD) {
            if (m == WM_SETFOCUS) SetPropW(h, kFocus, (HANDLE)1);
            else RemovePropW(h, kFocus);
            RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
        }
        InvalidateRect(h, nullptr, TRUE);
        break;
    case WM_ENABLE:
        InvalidateRect(h, nullptr, TRUE);
        break;
    case WM_DESTROY:
        RemovePropW(h, kHover);
        RemovePropW(h, kFocus);
        RemovePropW(h, kOld);
        if (sk) { RemovePropW(h, kSkin); delete sk; }
        break;
    }
    return old ? CallWindowProcW(old, h, m, w, l) : DefWindowProcW(h, m, w, l);
}

// --- tür → kind çözümü ------------------------------------------------------------
static int KindOfType(const wchar_t* type) {
    if      (_wcsicmp(type, L"Button")      == 0) return K_BUTTON;
    else if (_wcsicmp(type, L"CheckBox")    == 0) return K_CHECK;
    else if (_wcsicmp(type, L"RadioButton") == 0) return K_RADIO;
    else if (_wcsicmp(type, L"GroupBox")    == 0) return K_GROUP;
    // Alan derisi (kenar): metin/lista denetimleri — mac/modern/grafit görünümde çerçeve
    else if (_wcsicmp(type, L"TextBox")      == 0 ||
             _wcsicmp(type, L"MaskedTextBox") == 0 ||
             _wcsicmp(type, L"NumericUpDown") == 0 ||
             _wcsicmp(type, L"ListBox")       == 0 ||
             _wcsicmp(type, L"CheckedListBox") == 0) return K_FIELD;
    return -1;
}

// --- uygula / yerinde güncelle / yerinden sök --------------------------------------
// alan (TextBox/ListBox) denetiminin sistem kenarını al / geri ver — çerçeveyi
// tema kendi çizsin (mac yuvarlak mavi-odak, modern düz tek çizgi, grafit tema kenarı)
static void FieldSystemEdge(HWND h, bool give) {
    LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    LONG_PTR st = GetWindowLongPtrW(h, GWL_STYLE);
    LONG_PTR nex = give ? (ex | WS_EX_CLIENTEDGE)   : (ex & ~(LONG_PTR)WS_EX_CLIENTEDGE);
    LONG_PTR nst = give ? (st | WS_BORDER)          : (st & ~(LONG_PTR)WS_BORDER);
    if (nex != ex) SetWindowLongPtrW(h, GWL_EXSTYLE, nex);
    if (nst != st) SetWindowLongPtrW(h, GWL_STYLE, nst);
    if (nex != ex || nst != st)
        SetWindowPos(h, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool Apply(HWND h, const wchar_t* type, int theme) {
    if (!h || !IsWindow(h)) return false;
    int kind = KindOfType(type);
    if (kind < 0) return false;
    int flavor = 0;
    if      (theme == theme::APPLEMAC) flavor = FL_MAC;
    else if (theme == theme::MODERN)   flavor = FL_MODERN;
    else if (theme == theme::RUBY)     flavor = FL_RUBY;      // M2God grafit 4'lüsü
    else if (theme == theme::YELLOW)   flavor = FL_YELLOW;
    else if (theme == theme::PURPLE)   flavor = FL_PURPLE;
    else if (theme == theme::DARKM)    flavor = FL_DARK;

    Skin* sk = (Skin*)GetPropW(h, kSkin);
    if (flavor == 0) {
        // OLDSCHOOL: deriyi YERİNDEN SÖK — prosedür zinciri passthrough kalır
        // (kOld prop'U DURUR; SkinProc eski proc'a aktarmaya devam eder)
        if (sk) {
            RemovePropW(h, kSkin);
            bool wasField = (sk->kind == K_FIELD);
            delete sk;
            if (wasField) FieldSystemEdge(h, true);   // sistem 3D kenarını geri ver
            InvalidateRect(h, nullptr, TRUE);
        }
        return true;
    }

    if (sk) {   // zaten takılı — flavor YERİNDE güncelle (imha/yeniden yaratma yok)
        sk->kind = kind; sk->flavor = flavor;
        if (kind == K_FIELD) FieldSystemEdge(h, false);
        InvalidateRect(h, nullptr, TRUE);
        RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE);
        return true;
    }

    // yeni tak
    sk = new Skin{ kind, flavor };
    SetPropW(h, kSkin, sk);
    // KRİTİK: prosedür zincirine SADECE BİR KEZ gir (SkinProc zaten zincirdeyse
    // kOld'ü aşılamayla ezme — sonsuz özyineleme olur)
    WNDPROC cur = (WNDPROC)GetWindowLongPtrW(h, GWLP_WNDPROC);
    if (cur != SkinProc) {
        SetPropW(h, kOld, (HANDLE)cur);
        SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)SkinProc);
    }
    if (kind == K_FIELD) {
        FieldSystemEdge(h, false);
        // metin kenarlardan 4px içeride başlasın (mac alan içi boşluğu)
        SendMessageW(h, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4, 4));
    }
    InvalidateRect(h, nullptr, TRUE);
    return true;
}

} // namespace paintctl
