// themetest12.cpp — FAZ 2.12: yerinde tema geçişi + Aqua/Modern emisyon testi.
// Model kur (proje teması AppleMac; Button/CheckBox/RadioButton/GroupBox/TextBox/
// ListBox — açık AppleMac/Modern/kalıtsal karışık) → EmitProject → çıkan Form1.cpp
// Global.h ile mingw -std=c++14 (ve VS2019 varsayılanı) ile derlenebilmeli.
// RAD STUDIO SİLİNDİ — bu test artık RadCtl/IDRad/tema=3 YAYMAMALI.
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
    forms::SetThemeId(2);           // proje varsayılanı APPLE & MAC (FAZ 2.12: 2)
    g_model.formProps[L"Text"] = L"Theme12Test";
    g_model.formW = 720; g_model.formH = 520;

    // proje (mac) temalı denetimler — SetThemeAll(mac) derileri
    Ctrl* br = g_model.Add(L"Button", 20, 20);
    br->name = L"btnMac"; br->w = 150; br->h = 32;
    br->props[L"Text"] = L"Mac buton";
    br->events[L"Click"] = L"btnMac_Click";

    // açık MODERN temalı denetim — FAZ 2.12: ModernCtl::Apply ile KİLİTLİ deri
    // (menüden tema değişse bile nesne modern görünür)
    Ctrl* bm = g_model.Add(L"Button", 190, 20);
    bm->name = L"btnModern"; bm->w = 150; bm->h = 32;
    bm->props[L"Text"] = L"Modern";
    bm->props[L"Theme"] = L"Modern";

    // kalıtsal — AquaCtl::Apply YAZILMAMALI (yalnız açık AppleMac kilitli)
    Ctrl* bi = g_model.Add(L"Button", 360, 20);
    bi->name = L"btnInherit"; bi->w = 150; bi->h = 32;
    bi->props[L"Text"] = L"Kalitsal";

    // proje (mac) temalı checkbox/radio/groupbox + ALANLAR
    Ctrl* cb1 = g_model.Add(L"CheckBox", 20, 70);
    cb1->name = L"chkMac"; cb1->w = 160; cb1->h = 24;
    cb1->props[L"Text"] = L"mac onay";
    cb1->props[L"Checked"] = L"True";
    Ctrl* rd1 = g_model.Add(L"RadioButton", 20, 100);
    rd1->name = L"radMac"; rd1->w = 160; rd1->h = 24;
    rd1->props[L"Text"] = L"mac seçim";
    rd1->props[L"Checked"] = L"True";
    Ctrl* gb1 = g_model.Add(L"GroupBox", 20, 130);
    gb1->name = L"groupBoxMac"; gb1->w = 220; gb1->h = 100;
    gb1->props[L"Text"] = L"mac grup";
    Ctrl* cbm = g_model.Add(L"CheckBox", 16, 30, gb1);
    cbm->name = L"chkModern"; cbm->w = 170; cbm->h = 24;
    cbm->props[L"Text"] = L"modern onay";
    cbm->props[L"Theme"] = L"Modern";           // grup mac, çocuk modern
    Ctrl* tb = g_model.Add(L"TextBox", 20, 250);
    tb->name = L"txtMac"; tb->w = 180; tb->h = 26;
    tb->props[L"Text"] = L"mac alan";
    Ctrl* lb = g_model.Add(L"ListBox", 220, 250);
    lb->name = L"lstMac"; lb->w = 140; lb->h = 90;
    lb->props[L"Items"] = L"Öğe 1\nÖğe 2";
    // AÇIK AppleMac TextBox — kilitli AquaCtl::Apply beklentisi
    Ctrl* tb2 = g_model.Add(L"TextBox", 20, 300);
    tb2->name = L"txtLockedMac"; tb2->w = 180; tb2->h = 26;
    tb2->props[L"Text"] = L"kilitli mac";
    tb2->props[L"Theme"] = L"AppleMac";

    std::wstring dir = L"theme12_out";
    CreateDirectoryW(dir.c_str(), nullptr);
    bool ok = codegen::EmitProject(dir);
    printf("EmitProject: %s\n", ok ? "OK" : "FAIL");
    return ok ? 0 : 1;
}

// mainwnd.cpp bağlanmıyor (UI ağır) — surface'in kullandığı global burada
bool g_gridVisible = true;
