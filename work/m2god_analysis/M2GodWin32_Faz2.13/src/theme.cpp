// theme.cpp — FAZ 2.13: TEMA SİSTEMİ gerçeklemesi (bkz. theme.h)
// 0=Oldschool 1=Modern 2=Apple & Mac + M2God GRAFİT: 3=Ruby 4=Yellow
// 5=Purple 6=Dark (Faz2.5 M2God programının 4 teması — paintctl'de çizilir).
// Tema = saf paintctl çizimi; aktivasyon bağlamı / pencere imha YOK.
#include "theme.h"

namespace theme {

// --- durum -------------------------------------------------------------------
static int   s_theme = OLDSCHOOL;        // şu an etkin editör teması
static int   s_projectTheme = OLDSCHOOL; // projenin varsayılanı (kod üretimi)

int  Current() { return s_theme; }
int  Project() { return s_projectTheme; }
void SetProject(int t) { s_projectTheme = t; }

bool IsGraphite(int t) { return t >= FIRST_GRAPHITE && t <= LAST_THEME; }

// comctl32 v6 görsel stiller manifest'i (üretilen uygulamanın Global.h'ında
// bilgi amaçlı durur — tasarımcı çalışma anında bağlam kullanmaz)
const char* ManifestXmlA() {
    return
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
    "  <assemblyIdentity version=\"1.0.0.0\" processorArchitecture=\"*\" name=\"M2GodWin32.App\" type=\"win32\"/>\n"
    "  <description>M2God Win32 Application</description>\n"
    "  <dependency>\n"
    "    <dependentAssembly>\n"
    "      <assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\"\n"
    "        processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>\n"
    "    </dependentAssembly>\n"
    "  </dependency>\n"
    "  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">\n"
    "    <application>\n"
    "      <!-- Windows 10 ve 11 -->\n"
    "      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>\n"
    "      <!-- Windows 8.1 -->\n"
    "      <supportedOS Id=\"{1f676c76-80e1-4239-95bb-83d0f6d0da78}\"/>\n"
    "      <!-- Windows 8 -->\n"
    "      <supportedOS Id=\"{4a2f28e8-53b9-4d0c-a2c5-a0ab6d3c25b9}\"/>\n"
    "      <!-- Windows 7 -->\n"
    "      <supportedOS Id=\"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}\"/>\n"
    "    </application>\n"
    "  </compatibility>\n"
    "</assembly>\n";
}

void Switch(int t, HWND mainWnd) {
    if (t < OLDSCHOOL || t > LAST_THEME) t = OLDSCHOOL;
    s_theme = t;
    if (mainWnd && IsWindow(mainWnd)) DrawMenuBar(mainWnd);
}

// --- ad çözümleme ---------------------------------------------------------------
int IdOfName(const wchar_t* name) {
    if (!name || !*name) return -1;                     // boş → kalıtsal (HATA DÜZELTMESİ)
    if (_wcsicmp(name, L"Modern") == 0) return MODERN;
    if (_wcsicmp(name, L"AppleMac") == 0 || _wcsicmp(name, L"Apple & Mac") == 0 ||
        _wcsicmp(name, L"Apple") == 0 || _wcsicmp(name, L"Mac") == 0) return APPLEMAC;
    // M2God grafit temaları — kısa ve uzun adlar kabul (Faz2.5 combo1 birebir)
    if (_wcsicmp(name, L"Ruby") == 0 || _wcsicmp(name, L"Ruby Graphite") == 0 ||
        _wcsicmp(name, L"RubyGraphite") == 0) return RUBY;
    if (_wcsicmp(name, L"Yellow") == 0 || _wcsicmp(name, L"Yellow Graphite") == 0 ||
        _wcsicmp(name, L"YellowGraphite") == 0) return YELLOW;
    if (_wcsicmp(name, L"Purple") == 0 || _wcsicmp(name, L"Purple Graphite") == 0 ||
        _wcsicmp(name, L"PurpleGraphite") == 0) return PURPLE;
    if (_wcsicmp(name, L"Dark") == 0) return DARKM;
    if (_wcsicmp(name, L"Oldschool") == 0) return OLDSCHOOL;
    return -1;    // bilinmeyen ad — "(Proje Varsayılanı)", "RadStudio"... → kalıtsal
}

const wchar_t* NameOf(int t) {
    switch (t) {
    case MODERN:    return L"Modern";
    case APPLEMAC:  return L"Apple & Mac";
    case RUBY:      return L"Ruby Graphite";
    case YELLOW:    return L"Yellow Graphite";
    case PURPLE:    return L"Purple Graphite";
    case DARKM:     return L"Dark";
    default:        return L"Oldschool";
    }
}

const wchar_t* SaveNameOf(int t) {
    switch (t) {
    case MODERN:    return L"Modern";
    case APPLEMAC:  return L"AppleMac";
    case RUBY:      return L"Ruby";
    case YELLOW:    return L"Yellow";
    case PURPLE:    return L"Purple";
    case DARKM:     return L"Dark";
    default:        return L"Oldschool";
    }
}

const wchar_t* MenuNameOf(int t) {
    switch (t) {
    case MODERN:    return L"Modern (.NET 4.8 / WinForm benzeri)";
    case APPLEMAC:  return L"Apple & Mac (Aqua — kapsül buton, trafik ışıkları)";
    case RUBY:      return L"Ruby Graphite (M2God — kırmızı gradyan butonlar)";
    case YELLOW:    return L"Yellow Graphite (M2God — altın gradyan butonlar)";
    case PURPLE:    return L"Purple Graphite (M2God — mor grafit panel)";
    case DARKM:     return L"Dark (M2God — koyu grafit)";
    default:        return L"Oldschool (Klasik Win32) — VARSAYILAN";
    }
}

// --- M2God grafit temaların başlık çubuğu gradyanı (Faz2.5 theme.h birebir) ----
// Ruby : tbBase1(118,3,0) tbBase2(53,1,0)
// Yellow: (117,81,0)/(56,38,0)   Purple: (60,15,30)/(40,10,20)   Dark: (45,45,45)/(20,20,20)
COLORREF TitleGradTop(int t) {
    switch (t) {
    case RUBY:   return RGB(118, 3, 0);
    case YELLOW: return RGB(117, 81, 0);
    case PURPLE: return RGB(60, 15, 30);
    case DARKM:  return RGB(45, 45, 45);
    default:     return RGB(236, 236, 236);
    }
}
COLORREF TitleGradBottom(int t) {
    switch (t) {
    case RUBY:   return RGB(53, 1, 0);
    case YELLOW: return RGB(56, 38, 0);
    case PURPLE: return RGB(40, 10, 20);
    case DARKM:  return RGB(20, 20, 20);
    default:     return RGB(210, 210, 212);
    }
}

COLORREF FormBackColorOf(int t) {
    switch (t) {
    case APPLEMAC:  return RGB(236, 236, 236);   // mac pencere zemini #ECECEC
    case MODERN:    return RGB(240, 240, 240);   // WinForms Control zemini #F0F0F0
    case RUBY:
    case YELLOW:
    case PURPLE:
    case DARKM:     return RGB(37, 37, 37);      // M2God CLR_FORM_BACK #252525
    default:        return RGB(255, 255, 255);   // eski davranış: beyaz
    }
}

int Effective(const wchar_t* themeVal) {
    int id = IdOfName(themeVal);
    if (id >= 0) return id;
    return s_theme;   // (Proje Varsayılanı) / boş / bilinmeyen → editörün teması
}

} // namespace theme
