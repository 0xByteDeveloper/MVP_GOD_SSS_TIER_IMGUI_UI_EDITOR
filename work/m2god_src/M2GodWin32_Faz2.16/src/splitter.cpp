// splitter.cpp — FAZ 2.8: sürüklenebilir dikey ayraç gerçeklemesi
// Paneller arasındaki 6px'lik çubuk: imleç SIZENS olur, basılı tutup
// sürükleyince üst panel yüksekliği değişir (mainwnd::RefreshLayout →
// DoLayout kıstırma uygulayıp tüm sütunu yeniden dizer).
// ÇİFT TIK → varsayılan yüksekliğe döner.
#include "splitter.h"
#include "mainwnd.h"

namespace splitter {

static const int SPLIT_W = 6;
static const int DEF_LEFT = 260;     // sol üst (Kod Gezgini) varsayılan yükseklik
static int s_h[2] = { DEF_LEFT, FORMLIST_H };   // üst panel yükseklikleri

int TopHeight(int side) { return (side == 0) ? s_h[0] : s_h[1]; }
int& TopHeightRef(int side) { return s_h[side]; }

struct DragState {
    bool on = false;
    int  side = 0;
    int  startY = 0;      // fare başlangıç (EKRAN koordinatı)
    int  startTop = 0;    // sürükleme başlangıcındaki üst panel yüksekliği
};
static DragState s_drag;

static LRESULT CALLBACK SplitProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) {
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return TRUE;
        }
        break;
    case WM_LBUTTONDOWN: {
        SetFocus(GetParent(h));
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        ClientToScreen(h, &pt);
        int side = (int)GetWindowLongPtrW(h, GWLP_USERDATA);
        s_drag.on = true;
        s_drag.side = side;
        s_drag.startY = pt.y;
        s_drag.startTop = s_h[side];
        SetCapture(h);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (s_drag.on && (w & MK_LBUTTON)) {
            POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            ClientToScreen(h, &pt);
            s_h[s_drag.side] = s_drag.startTop + (pt.y - s_drag.startY);
            mainwnd::RefreshLayout();   // DoLayout kıstırıp dizmesi için
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (s_drag.on) { s_drag.on = false; ReleaseCapture(); }
        return 0;
    case WM_CAPTURECHANGED:
        s_drag.on = false;
        return 0;
    case WM_LBUTTONDBLCLK: {   // çift tık → varsayılan yükseklik
        int side = (int)GetWindowLongPtrW(h, GWLP_USERDATA);
        s_h[side] = (side == 0) ? DEF_LEFT : FORMLIST_H;
        mainwnd::RefreshLayout();
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(212, 212, 212));
        FillRect(dc, &rc, br); DeleteObject(br);
        // kavrama noktaları (grip) — dikey ortada 3 nokta
        int cx = rc.right / 2, cy = rc.bottom / 2;
        HBRUSH db = CreateSolidBrush(RGB(120, 120, 120));
        for (int i = -1; i <= 1; i++) {
            RECT d = { cx - 2, cy + i * 4 - 1, cx + 3, cy + i * 4 + 3 };
            FillRect(dc, &d, db);
        }
        DeleteObject(db);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

HWND Create(HWND parent, int side) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = SplitProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_SIZENS);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"M2GOD_SPLIT";
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(0, L"M2GOD_SPLIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                             0, 0, SPLIT_W, 100, parent, (HMENU)(1530 + side), g_hInst, nullptr);
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)side);
    return h;
}

} // namespace splitter
