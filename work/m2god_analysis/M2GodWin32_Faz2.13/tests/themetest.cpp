// themetest.cpp — FAZ 2.10: nesne özel tema + tema geçişi emisyon testi.
// Konsol (hstub) ana programı: model kur (4 düğme: Oldschool/Modern/Apple & Mac/
// kalıtsal) → proje klasörüne üret (EmitProject) → çıkan Form1.cpp Global.h ile birlikte
// mingw ile derlenebilir olmalı (derleme ayrı adımda yapılır).
#include "app.h"
#include "catalog.h"
#include "model.h"
#include "forms.h"
#include "codegen.h"
#include <windows.h>
#include <stdio.h>

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_catalog.Load(L"");          // yerleşik katalog
    forms::InitNew();
    forms::SetThemeId(0);           // proje varsayılanı: Oldschool
    g_model.formProps[L"Text"] = L"ThemeTest";
    g_model.formW = 640; g_model.formH = 480;
    g_model.formEvents[L"Load"] = L"Form1_Load";

    // 1) btnOld — proje Oldschool + nesne AÇIKÇA Oldschool → yine sarılmalı
    Ctrl* b1 = g_model.Add(L"Button", 20, 20);
    b1->name = L"btnOld"; b1->w = 140; b1->h = 32;
    b1->props[L"Text"] = L"Klasik";
    b1->props[L"Theme"] = L"Oldschool";
    b1->events[L"Click"] = L"btnOld_Click";

    // 2) btnMod — proje tersi Modern → sarılmalı
    Ctrl* b2 = g_model.Add(L"Button", 180, 20);
    b2->name = L"btnMod"; b2->w = 140; b2->h = 32;
    b2->props[L"Text"] = L"Modern";
    b2->props[L"Theme"] = L"Modern";
    b2->events[L"Click"] = L"btnMod_Click";

    // 3) btnMac — Apple & Mac (mac buton çizimi + comctl6) → sarılmalı + MacStyle::Apply
    Ctrl* b4 = g_model.Add(L"Button", 340, 60);
    b4->name = L"btnMac"; b4->w = 140; b4->h = 32;
    b4->props[L"Text"] = L"Mac";
    b4->props[L"Theme"] = L"AppleMac";
    b4->events[L"Click"] = L"btnMac_Click";

    // 4) btnInherit — kalıtsal → sarılmamalı
    Ctrl* b3 = g_model.Add(L"Button", 340, 20);
    b3->name = L"btnInherit"; b3->w = 140; b3->h = 32;
    b3->props[L"Text"] = L"Kalitsal";

    // 5) groupBox1 içine çocuk — konteyner çocuğu da kendi temasını taşıyabilmeli
    Ctrl* gb = g_model.Add(L"GroupBox", 20, 90);
    gb->name = L"groupBox1"; gb->w = 240; gb->h = 120;
    gb->props[L"Text"] = L"Ayarlar";
    gb->props[L"Theme"] = L"Modern";
    Ctrl* cb = g_model.Add(L"CheckBox", 16, 30, gb);
    cb->name = L"chkOld"; cb->w = 180; cb->h = 24;
    cb->props[L"Text"] = L"Klasik onay kutusu";
    cb->props[L"Theme"] = L"Oldschool";

    std::wstring dir = L"theme_out";
    CreateDirectoryW(dir.c_str(), nullptr);
    bool ok = codegen::EmitProject(dir);
    printf("EmitProject: %s\n", ok ? "OK" : "FAIL");
    return ok ? 0 : 1;
}

// mainwnd.cpp bağlanmıyor (UI ağır) — surface'in kullandığı global burada
bool g_gridVisible = true;
