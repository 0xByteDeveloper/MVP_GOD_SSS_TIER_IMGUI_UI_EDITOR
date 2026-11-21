// formlist.cpp — FAZ 2.7: FORMLAR listesi paneli
// Sağ sütunun ÜST yarısı: başlık ("Formlar") + "+ Yeni Form" düğmesi + ListView.
// Alt yarısı Özellikler panelidir (mainwnd DoLayout ayarlar).
#include "formlist.h"
#include "app.h"
#include "forms.h"
#include <commctrl.h>

namespace formlist {

static HWND s_panel = nullptr;
static HWND s_caption = nullptr;
static HWND s_btnNew = nullptr;
static HWND s_list = nullptr;
static bool s_internal = false;     // programatik seçim (notif yayma)

// --- liste seçimi → ana pencereye form geçişi bildir ---------------------------
static void NotifySwitch(int index) {
    if (s_internal) return;
    if (index < 0 || index >= forms::Count()) return;
    PostMessageW(g_hMain, MWM_FORM_SWITCH, (WPARAM)index, 0);
}

// --- sağ tık menüsü ---------------------------------------------------------------
static void ShowContextMenu(HWND h) {
    // tıklanan öğe
    LVHITTESTINFO ht = { 0 };
    DWORD mp = GetMessagePos();
    ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
    ScreenToClient(s_list, &ht.pt);
    int idx = (int)SendMessageW(s_list, LVM_HITTEST, 0, (LPARAM)&ht);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, M2CMD_FORM_NEW,    L"Yeni Form\tCtrl+Shift+N");
    if (idx >= 0) {
        AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(m, MF_STRING, M2CMD_FORM_RENAME, L"Yeniden Adlandır...\tF2");
        AppendMenuW(m, MF_STRING, M2CMD_FORM_DELETE, L"Sil");
    }
    if (idx >= 0) SendMessageW(s_list, LVM_SETSELECTIONMARK, 0, idx);
    POINT pt = { GET_X_LPARAM(mp), GET_Y_LPARAM(mp) };
    int cmd = TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, h, nullptr);
    DestroyMenu(m);
    if (cmd == M2CMD_FORM_NEW || cmd == M2CMD_FORM_RENAME)
        PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
    else if (cmd == M2CMD_FORM_DELETE && idx >= 0)
        PostMessageW(g_hMain, MWM_FORM_DELETE, (WPARAM)idx, 0);
}

static LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == IDC_FORMLIST_NEW) {                    // "+ Yeni Form"
            PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_FORM_NEW, 0), 0);
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom != IDC_FORMLIST_LIST) break;
        if (nm->code == NM_CLICK || nm->code == NM_DBLCLK) {
            // tek tık → form geçişi; çift tık → yeniden adlandır (VS F2 davranışı)
            LVHITTESTINFO ht = { 0 };
            DWORD mp = GetMessagePos();
            ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
            ScreenToClient(s_list, &ht.pt);
            int idx = (int)SendMessageW(s_list, LVM_HITTEST, 0, (LPARAM)&ht);
            if (idx >= 0) {
                if (nm->code == NM_CLICK) NotifySwitch(idx);
                else PostMessageW(g_hMain, WM_COMMAND, MAKEWPARAM(M2CMD_FORM_RENAME, 0), 0);
            }
            return 1;
        }
        if (nm->code == LVN_ITEMCHANGED) {               // klavye ile seçim
            NMLISTVIEW* nl = (NMLISTVIEW*)l;
            if ((nl->uNewState & LVIS_SELECTED) && !(nl->uOldState & LVIS_SELECTED) && !s_internal)
                NotifySwitch(nl->iItem);
            return 0;
        }
        break;
    }
    case WM_CONTEXTMENU: {
        if ((HWND)w == s_list) { ShowContextMenu(h); return 0; }
        break;
    }
    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        MoveWindow(s_caption, 0, 0, rc.right, 26, TRUE);
        MoveWindow(s_btnNew, 4, 30, rc.right - 8, 26, TRUE);
        int listH = rc.bottom - 60; if (listH < 20) listH = 20;
        MoveWindow(s_list, 0, 60, rc.right, listH, TRUE);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(dc, &rc, br); DeleteObject(br);
        RECT cap = { 0, 0, rc.right, 26 };
        HBRUSH capb = CreateSolidBrush(RGB(0, 120, 215));
        FillRect(dc, &cap, capb); DeleteObject(capb);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255));
        SelectObject(dc, g_fontUIBold);
        RECT tr = cap; tr.left += 10;
        DrawTextW(dc, L"Formlar", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        EndPaint(h, &ps);
        return 0;
    }
    }
    return DefWindowProcW(h, m, w, l);
}

HWND Create(HWND parent) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = PanelProc; wc.hInstance = g_hInst; wc.hbrBackground = nullptr;
        wc.lpszClassName = L"M2GOD_FORMLIST";
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(0, L"M2GOD_FORMLIST", L"", WS_CHILD | WS_VISIBLE,
                             0, 0, PROPS_W, FORMLIST_H, parent, (HMENU)1520, g_hInst, nullptr);
    s_panel = h;
    s_caption = CreateWindowExW(0, L"STATIC", L"Formlar", WS_CHILD | WS_VISIBLE,
                                 0, 0, PROPS_W, 26, h, nullptr, g_hInst, nullptr);
    s_btnNew = CreateWindowExW(0, L"BUTTON", L"+ Yeni Form",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                               4, 30, PROPS_W - 8, 26, h, (HMENU)IDC_FORMLIST_NEW, g_hInst, nullptr);
    s_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER |
                              LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
                              0, 60, PROPS_W, 100, h, (HMENU)IDC_FORMLIST_LIST, g_hInst, nullptr);
    SendMessageW(s_list, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(s_btnNew, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SendMessageW(s_caption, WM_SETFONT, (WPARAM)g_fontUIBold, TRUE);
    // tek sütun (başlık gizli)
    LVCOLUMNW col = { 0 };
    col.mask = LVCF_WIDTH;
    col.cx = PROPS_W - 28;
    SendMessageW(s_list, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);
    // tam satır seçimi + küçük simge hizası
    DWORD st = (DWORD)SendMessageW(s_list, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessageW(s_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, st | LVS_EX_FULLROWSELECT);
    Refresh();
    return h;
}

void Refresh() {
    if (!s_list || !IsWindow(s_list)) return;
    s_internal = true;
    SendMessageW(s_list, LVM_DELETEALLITEMS, 0, 0);
    auto names = forms::Names();
    for (size_t i = 0; i < names.size(); i++) {
        LVITEMW it = { 0 };
        it.mask = LVIF_TEXT;
        it.iItem = (int)i;
        it.pszText = (LPWSTR)names[i].c_str();
        SendMessageW(s_list, LVM_INSERTITEMW, 0, (LPARAM)&it);
    }
    int active = forms::ActiveIndex();
    if (active >= 0 && active < (int)names.size()) {
        LVITEMW it = { 0 };
        it.mask = LVIF_STATE;
        it.iItem = active;
        it.state = LVIS_SELECTED | LVIS_FOCUSED;
        it.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        SendMessageW(s_list, LVM_SETITEMW, 0, (LPARAM)&it);
        SendMessageW(s_list, LVM_ENSUREVISIBLE, (WPARAM)active, FALSE);
    }
    s_internal = false;
}

void Select(int index) {
    if (!s_list || !IsWindow(s_list)) return;
    s_internal = true;
    if (index >= 0 && index < forms::Count()) {
        LVITEMW it = { 0 };
        it.mask = LVIF_STATE;
        it.iItem = index;
        it.state = LVIS_SELECTED | LVIS_FOCUSED;
        it.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        SendMessageW(s_list, LVM_SETITEMW, 0, (LPARAM)&it);
        SendMessageW(s_list, LVM_ENSUREVISIBLE, (WPARAM)index, FALSE);
    }
    s_internal = false;
}

} // namespace formlist
