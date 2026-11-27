// themetest13.cpp — FAZ 2.13: M2God grafit 4'lüsü + klasör projesi testi.
// 1) Proje teması Ruby (3) → üretilen Form1.cpp GraphiteCtl::Apply(btn, 3)
//    içermeli, RadCtl/eski temalar YAYMAMALI; Global.h ile C++14 derlenmeli.
// 2) forms::SaveFolderProject + LoadFolderProject gidiş-dönüşü: Forms.info +
//    FormN.info yazılır, geri yüklenince form sayısı/adlar/tema korunur.
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "forms.h"
#include "codegen.h"
#include "theme.h"
#include <windows.h>
#include <stdio.h>

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_catalog.Load(L"");          // yerleşik katalog
    forms::InitNew();

    // --- 1) tema kimlik çözümleme ---
    if (theme::IdOfName(L"Ruby") != 3 || theme::IdOfName(L"Ruby Graphite") != 3 ||
        theme::IdOfName(L"Yellow") != 4 || theme::IdOfName(L"Purple") != 5 ||
        theme::IdOfName(L"Dark") != 6 || theme::IdOfName(L"Oldschool") != 0)
        return 1;
    if (!theme::IsGraphite(3) || !theme::IsGraphite(6) || theme::IsGraphite(2) || theme::IsGraphite(7))
        return 2;
    if (theme::FormBackColorOf(3) != RGB(37,37,37) || theme::FormBackColorOf(0) != RGB(255,255,255))
        return 3;

    // --- 2) kod üretimi: Ruby projede açık/kalıtsal/kilitli karışık ---
    forms::SetThemeId(3);           // Ruby Graphite
    g_model.formProps[L"Text"] = L"Graphite13Test";
    g_model.formW = 700; g_model.formH = 500;

    Ctrl* b1 = g_model.Add(L"Button", 20, 20);
    b1->name = L"btnRuby"; b1->w = 150; b1->h = 32;
    b1->props[L"Text"] = L"Ruby buton";

    Ctrl* b2 = g_model.Add(L"Button", 190, 20);
    b2->name = L"btnDark"; b2->w = 150; b2->h = 32;
    b2->props[L"Text"] = L"Dark kilitli";
    b2->props[L"Theme"] = L"Dark";      // açık seçim → GraphiteCtl::Apply(.., 6)

    Ctrl* cb = g_model.Add(L"CheckBox", 20, 70);
    cb->name = L"chk1"; cb->props[L"Text"] = L"Kontrol";
    Ctrl* tx = g_model.Add(L"TextBox", 20, 110);
    tx->name = L"txt1"; tx->w = 200; tx->h = 26;
    tx->props[L"Text"] = L"alan";
    Ctrl* gb = g_model.Add(L"GroupBox", 20, 150);
    gb->name = L"grp1"; gb->w = 260; gb->h = 120; gb->props[L"Text"] = L"Grup";
    Ctrl* cmb = g_model.Add(L"ComboBox", 300, 70);
    cmb->name = L"combo1"; cmb->w = 170; cmb->h = 21;
    cmb->props[L"Items"] = L"Ruby Graphite\nYellow Graphite\nPurple\nDark";

    if (!codegen::EmitForm(L"Z:\\__m2god_t13", true)) return 10;
    if (!codegen::EmitProject(L"Z:\\__m2god_t13")) return 11;

    // --- 3) klasör projesi gidiş-dönüşü ---
    forms::AddNew();                  // Form2 ekle
    if (forms::Count() != 2) return 20;
    if (!forms::SaveFolderProject(L"Z:\\__m2god_t13")) return 21;
    // Forms.info + Form1.info + Form2.info yazılmış olmalı
    if (GetFileAttributesW(L"Z:\\__m2god_t13\\Forms.info") == INVALID_FILE_ATTRIBUTES) return 22;
    if (GetFileAttributesW(L"Z:\\__m2god_t13\\Form1.info") == INVALID_FILE_ATTRIBUTES) return 23;
    if (GetFileAttributesW(L"Z:\\__m2god_t13\\Form2.info") == INVALID_FILE_ATTRIBUTES) return 24;
    // taze yükle: 2 form + tema Ruby korunmalı
    forms::InitNew();
    if (!forms::LoadFolderProject(L"Z:\\__m2god_t13")) return 25;
    if (forms::Count() != 2) return 26;
    if (forms::ThemeId() != 3) return 27;
    auto names = forms::Names();
    if (names.size() != 2 || names[0].empty()) return 28;
    // yürürlükteki aktif formda denetimler geri gelmeli
    int total = 0;
    for (int i = 0; i < forms::Count(); i++) {
        forms::SwitchTo(i);
        total += (int)g_model.ctrls.size();
    }
    if (total != 6) return 28;        // 6 denetim (Form1) + Form2 boş
    return 0;                          // tüm kontroller geçti
}
