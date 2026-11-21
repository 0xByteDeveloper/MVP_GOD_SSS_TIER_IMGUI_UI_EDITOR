// codetree.cpp — FAZ 2.9: KOD GEZGİNİ (sol üst panel) — TAM YENİDEN YAZIM
// Kullanıcı isteği: "FORMLAR GÖZÜKÜR, AŞAĞI DOĞRU GENİŞLET ŞEKLİNDE AÇILIR;
// İÇİNDEKİLER FORM1 İÇİNDEKİ OBJELER; MyButton1'e tıklarsam UI editörde o
// seçilecek, özellikler/olaylar kısmında MyButton1'e ait şeyler görünür;
// MyButton1'i genişletirsem eventleri (MyButton1_Click) gözükür."
//
// Yapı (VS2019 Solution Explorer / RAD Studio Structure tarzı):
//   Form1            (kalın = aktif form; TÜM formlar genişletilebilir)
//   ├─ button1       → tıkla: editörde seç + Özellikler/Olaylar panelinde satırları
//   │   ├─ (alt kontroller — konteyner ise: Panel/GroupBox/TabControl sayfaları)
//   │   └─ button1_Click  → tıkla: Olaylar sekmesi; çift tık: KODA GİT
//   ├─ label1
//   ├─ timer1        (bileşenler de listelenir)
//   └─ Form1_Load    (form düzeyi olaylar sonda)
#include "codetree.h"
#include "forms.h"
#include "model.h"
#include <commctrl.h>
#include <set>
#include <map>

namespace codetree {

static HWND s_panel = nullptr;
static HWND s_tree = nullptr;
static bool s_internal = false;   // programatik seçim → bildirim yayma

// ağaç düğümü yükü: HTREEITEM → bilgi
struct Node {
    int  formIdx = 0;
    bool isForm = false;          // form düğümü
    bool isHandler = false;       // olay işleyici düğümü
    wchar_t ctrlName[64] = L"";   // boş → form düzeyi olay
    wchar_t evName[64] = L"";
    wchar_t handler[96] = L"";
};
static std::map<HTREEITEM, Node> s_nodes;

// genişletme durumu hatırlanır: "F|Form1" (form), "O|Form1|button1" (obje)
// FAZ 2.9: İKİ küme — s_expanded (açık bırakıldı), s_collapsed (kapatıldı).
// Hiç dokunulmamış form: AKTİF form otomatik açılır (keşfedilebilirlik).
static std::set<std::wstring> s_expanded;
static std::set<std::wstring> s_collapsed;
static std::wstring FormKey(const std::wstring& form) { return L"F|" + form; }
static std::wstring ObjKey(const std::wstring& form, const std::wstring& obj) {
    return L"O|" + form + L"|" + obj;
}
static bool WantExpanded(const std::wstring& key, bool activeForm) {
    if (s_collapsed.count(key)) return false;
    return s_expanded.count(key) > 0 || activeForm;
}

// --- gezinme bildirimleri ana pencereye ------------------------------------------
static void PostNav(const Node& nd, UINT msg) {
    NavInfo* ni = new NavInfo();
    ni->formIdx = nd.formIdx;
    ni->isForm = nd.isForm;
    ni->isHandler = nd.isHandler;
    wcsncpy(ni->ctrlName, nd.ctrlName, 63);
    wcsncpy(ni->evName, nd.evName, 63);
    wcsncpy(ni->handler, nd.handler, 95);
    PostMessageW(g_hMain, msg, 0, (LPARAM)ni);
}

// --- ağaç doldurma ----------------------------------------------------------------
static HTREEITEM InsertItem(HWND tv, const wchar_t* text, HTREEITEM parent, bool bold) {
    TVINSERTSTRUCTW ti = { 0 };
    ti.hParent = parent;
    ti.hInsertAfter = TVI_LAST;
    ti.item.mask = TVIF_TEXT | TVIF_PARAM;
    ti.item.pszText = (LPWSTR)text;
    if (bold) { ti.item.mask |= TVIF_STATE; ti.item.state = TVIS_BOLD; ti.item.stateMask = TVIS_BOLD; }
    return (HTREEITEM)SendMessageW(tv, TVM_INSERTITEMW, 0, (LPARAM)&ti);
}

void Refresh() {
    if (!s_tree || !IsWindow(s_tree)) return;
    s_internal = true;
    SendMessageW(s_tree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    s_nodes.clear();

    auto names = forms::Names();
    int active = forms::ActiveIndex();
    HTREEITEM selectMe = nullptr;

    // aktif seçim → karşılık gelen düğüm seçilsin
    std::wstring selCtrl;
    bool selForm = g_model.formSelected || !g_model.selected;
    if (!selForm && g_model.selected) selCtrl = g_model.selected->name;

    for (int fi = 0; fi < (int)names.size(); fi++) {
        Node fn;
        fn.formIdx = fi; fn.isForm = true;
        HTREEITEM hForm = InsertItem(s_tree, names[fi].c_str(), TVI_ROOT, fi == active);
        if (hForm) s_nodes[hForm] = fn;
        if (fi == active && selForm) selectMe = hForm;

        // FAZ 2.9: TÜM formların TÜM objeleri listelenir (parklı formlar
        // tasarım JSON'undan okunur — aktif form değişmeden görülür).
        auto objs = forms::TreeObjectsOf(fi);
        int n = (int)objs.size();
        std::vector<HTREEITEM> hObj(n, nullptr);
        std::vector<bool> done(n, false);
        // önce ebeveynler çözülene dek döngü: konteyner (Panel/GroupBox/TabPage)
        // altına çocukları gömülür — VS Solution Explorer hiyerarşisi
        for (int guard = 0; guard < n + 1; guard++) {
            bool progress = false;
            for (int i = 0; i < n; i++) {
                if (done[i]) continue;
                if (objs[i].parentIdx >= 0 && (objs[i].parentIdx >= n || !done[objs[i].parentIdx])) continue;
                HTREEITEM parent = (objs[i].parentIdx >= 0 && objs[i].parentIdx < n && hObj[objs[i].parentIdx])
                                   ? hObj[objs[i].parentIdx] : hForm;
                Node cn;
                cn.formIdx = fi; cn.isForm = false; cn.isHandler = false;
                wcsncpy(cn.ctrlName, objs[i].name.c_str(), 63);
                HTREEITEM hCtrl = InsertItem(s_tree, objs[i].name.c_str(), parent, false);
                if (hCtrl) {
                    s_nodes[hCtrl] = cn;
                    hObj[i] = hCtrl;
                    if (fi == active && objs[i].name == selCtrl) selectMe = hCtrl;
                    if (WantExpanded(ObjKey(names[fi], objs[i].name), false))
                        SendMessageW(s_tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hCtrl);
                }
                done[i] = true; progress = true;
            }
            if (!progress) break;
        }
        // her objenin olay işleyicileri (varsa) — obje düğümünün çocukları
        for (int i = 0; i < n; i++) {
            if (!hObj[i] || objs[i].events.empty()) continue;
            for (auto& ev : objs[i].events) {
                Node hn;
                hn.formIdx = fi; hn.isForm = false; hn.isHandler = true;
                wcsncpy(hn.ctrlName, objs[i].name.c_str(), 63);
                wcsncpy(hn.evName, ev.first.c_str(), 63);
                wcsncpy(hn.handler, ev.second.c_str(), 95);
                HTREEITEM h = InsertItem(s_tree, ev.second.c_str(), hObj[i], false);
                if (h) s_nodes[h] = hn;
            }
        }
        // form düzeyi olay işleyicileri (Form1_Load vb.) — objelerden sonra
        for (auto& ev : forms::TreeFormEventsOf(fi)) {
            Node hn;
            hn.formIdx = fi; hn.isForm = false; hn.isHandler = true;
            hn.ctrlName[0] = 0;
            wcsncpy(hn.evName, ev.first.c_str(), 63);
            wcsncpy(hn.handler, ev.second.c_str(), 95);
            HTREEITEM h = InsertItem(s_tree, ev.second.c_str(), hForm, false);
            if (h) s_nodes[h] = hn;
        }
        // form düğümü genişletme durumu (dokunulmamış aktif form otomatik açılır)
        if (WantExpanded(FormKey(names[fi]), fi == active))
            SendMessageW(s_tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hForm);
    }
    if (selectMe)
        SendMessageW(s_tree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)selectMe);
    s_internal = false;
}

// --- panel prosedürü ----------------------------------------------------------------
static LRESULT CALLBACK PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom != IDC_CODETREE_TREE) break;
        if (nm->code == TVN_SELCHANGED) {
            if (s_internal) return 0;
            NMTREEVIEWW* nt = (NMTREEVIEWW*)l;
            auto it = s_nodes.find(nt->itemNew.hItem);
            if (it != s_nodes.end()) PostNav(it->second, MWM_CODETREE_NAV);
            return 0;
        }
        if (nm->code == NM_CLICK) {
            // FAZ 2.9: seçili düğüme TEKRAR tıklayınca da gezinme tetiklensin
            // (TVN_SELCHANGED yalnız seçim DEĞİŞİNce gelir — kullanıcı aynı
            // form/objeye tıklayınca editör/panel senkronu yine çalışmalı).
            TVHITTESTINFO ht = { 0 };
            DWORD mp = GetMessagePos();
            ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
            ScreenToClient(s_tree, &ht.pt);
            HTREEITEM hit = (HTREEITEM)SendMessageW(s_tree, TVM_HITTEST, 0, (LPARAM)&ht);
            if (hit && hit == (HTREEITEM)SendMessageW(s_tree, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
                auto it = s_nodes.find(hit);
                if (it != s_nodes.end() && !it->second.isHandler)
                    PostNav(it->second, MWM_CODETREE_NAV);
            }
            break;   // normal tık işlemi sürsün (genişletme oku vs.)
        }
        if (nm->code == NM_DBLCLK) {
            // çift tık → KODA GİT (VS bağlıysa gerçek VS2019 kodu, değilse üretim)
            TVHITTESTINFO ht = { 0 };
            DWORD mp = GetMessagePos();
            ht.pt.x = GET_X_LPARAM(mp); ht.pt.y = GET_Y_LPARAM(mp);
            ScreenToClient(s_tree, &ht.pt);
            HTREEITEM hit = (HTREEITEM)SendMessageW(s_tree, TVM_HITTEST, 0, (LPARAM)&ht);
            auto it = s_nodes.find(hit);
            if (it != s_nodes.end() && it->second.isHandler)
                PostNav(it->second, MWM_CODETREE_GOTO);
            return 0;
        }
        if (nm->code == TVN_ITEMEXPANDED) {   // genişletme durumunu hatırla
            NMTREEVIEWW* nt = (NMTREEVIEWW*)l;
            auto it = s_nodes.find(nt->itemNew.hItem);
            if (it != s_nodes.end()) {
                auto names = forms::Names();
                std::wstring form = (it->second.formIdx >= 0 && it->second.formIdx < (int)names.size())
                                    ? names[it->second.formIdx] : L"";
                std::wstring key = it->second.isForm ? FormKey(form)
                                                     : ObjKey(form, it->second.ctrlName);
                if (nt->action == TVE_EXPAND) { s_expanded.insert(key); s_collapsed.erase(key); }
                else                          { s_collapsed.insert(key); s_expanded.erase(key); }
            }
            return 0;
        }
        break;
    }
    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        int treeH = rc.bottom - 26; if (treeH < 20) treeH = 20;
        MoveWindow(s_tree, 0, 26, rc.right, treeH, TRUE);
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
        DrawTextW(dc, L"Kod Gezgini", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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
        wc.lpszClassName = L"M2GOD_CODETREE";
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(0, L"M2GOD_CODETREE", L"", WS_CHILD | WS_VISIBLE,
                             0, 0, TOOLBOX_W, 240, parent, (HMENU)1524, g_hInst, nullptr);
    s_panel = h;
    s_tree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                              TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
                              TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP,
                              0, 26, TOOLBOX_W, 200, h, (HMENU)IDC_CODETREE_TREE, g_hInst, nullptr);
    SendMessageW(s_tree, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    Refresh();
    return h;
}

} // namespace codetree
