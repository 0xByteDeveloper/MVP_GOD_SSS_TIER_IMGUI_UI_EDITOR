// tmpl12_compile_check.cpp — Global.h (global_template.inc) FAZ 2.12 kontrolü:
// şablon gövdesi tests/tmpl_global.h'e çıkarılır (check_template.sh) ve bu dosya
// onu include eder; üretilen kodun çağıracağı API'leri kullanır
// (Theme::Startup/Set/BackColorOf/AttachMenu, CtlPaint::SkinApply/
// SetThemeAll, AquaCtl::Apply kilitli deri, PushOverride/PopOverride stub'ları).
// Derleme: mingw -std=c++14 (VS2019 varsayılanına denk).
#include "tmpl_global.h"

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    M2God::Theme::Startup(2);                    // tasarımın teması: Apple & Mac
    if (M2God::Theme::Cur == 0) M2God::Theme::Set(true);   // bool geriye uyum
    M2God::Theme::Set(1);                        // Modern'e geç (yerinde)
    COLORREF cb = M2God::Theme::BackColorOf(M2God::Theme::Cur);
    M2God::FormBackColorRef() = cb;

    // üretilen InitializeControls'ün çağırdığı satırlar
    HWND fakeParent = GetDesktopWindow();
    HWND hBtn  = CreateWindowExW(0, L"BUTTON", L"Test", WS_CHILD | BS_PUSHBUTTON,
                                 0, 0, 100, 30, fakeParent, (HMENU)1, hInst, nullptr);
    HWND hChk  = CreateWindowExW(0, L"BUTTON", L"Chk", WS_CHILD | BS_AUTOCHECKBOX,
                                 0, 0, 120, 24, fakeParent, (HMENU)2, hInst, nullptr);
    HWND hRd   = CreateWindowExW(0, L"BUTTON", L"Rad", WS_CHILD | BS_AUTORADIOBUTTON,
                                 0, 0, 120, 24, fakeParent, (HMENU)3, hInst, nullptr);
    HWND hGrp  = CreateWindowExW(0, L"BUTTON", L"Grp", WS_CHILD | BS_GROUPBOX,
                                 0, 0, 200, 100, fakeParent, (HMENU)4, hInst, nullptr);
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                 WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                 0, 0, 140, 26, fakeParent, (HMENU)5, hInst, nullptr);
    HWND hList = CreateWindowExW(0, L"LISTBOX", L"",
                                 WS_CHILD | WS_BORDER | LBS_NOTIFY,
                                 0, 0, 140, 90, fakeParent, (HMENU)6, hInst, nullptr);
    if (hBtn) {
        M2God::AquaCtl::Apply(hBtn);             // nesneye KİLİTLİ mac derisi
        M2God::AquaCtl::Apply(hChk);
        M2God::AquaCtl::Apply(hRd);
        M2God::AquaCtl::Apply(hGrp);
        M2God::AquaCtl::Apply(hEdit);           // alan (mac kenar)
    }
    M2God::ModernCtl::Apply(hBtn);              // kilitli MODERN deri
    M2God::PlainCtl::Apply(hList);              // kilitli PLAIN (deri yok)
    // startup'ta üretilen satır: TÜM denetimler aktif temaya geçer
    M2God::CtlPaint::SetThemeAll(M2God::Theme::Cur, fakeParent);
    // menüden tema değişimi: üretilen M2ThemeSwitch gövdesinin birebir karşılığı
    M2God::Theme::Set(2);
    M2God::CtlPaint::SetThemeAll(2, fakeParent);
    M2God::FormBackColorRef() = M2God::Theme::BackColorOf(2);
    {
        COLORREF cb2 = M2God::FormBackColorRef();
        HBRUSH nb = CreateSolidBrush(cb2);
        HBRUSH ob = (HBRUSH)SetClassLongPtrW(fakeParent, GCLP_HBRBACKGROUND, (LONG_PTR)nb);
        if (ob) DeleteObject(ob);
    }
    // geriye uyum stub'ları (eski üretilmiş kod hâlâ derlenir)
    bool pushed = M2God::Theme::PushOverride(2);
    if (pushed) M2God::Theme::PopOverride();
    // eski bool API'leri
    M2God::Theme::Set(false);
    M2God::Theme::Save(true);
    M2God::Theme::Recreating = false;
    M2God::Theme::Modern = (M2God::Theme::Cur != 0);

    if (hBtn)  DestroyWindow(hBtn);
    if (hChk)  DestroyWindow(hChk);
    if (hRd)   DestroyWindow(hRd);
    if (hGrp)  DestroyWindow(hGrp);
    if (hEdit) DestroyWindow(hEdit);
    if (hList) DestroyWindow(hList);
    return 0;
}
