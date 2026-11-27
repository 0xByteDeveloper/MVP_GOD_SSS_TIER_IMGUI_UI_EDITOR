// ptr_test16.cpp — FAZ 2.16 TASARIM HAFIZASI TESTİ (designmem birimi)
//
// Kullanıcının bildirdiği bug'ın TAM kökenini test eder:
//   "M2God Vs ye bağla dedim, 'Bu proje için kayıtlı tasarım var' dedi.
//    Form1.m2god, Form2.m2god olarak görüyorum... sürekli onu gösteriyor."
//
// Doğrulanan davranışlar:
//   T1  Describe: v3 çok formlu .m2god → doğru form/obje/tema sayısı
//   T2  Describe: BOŞ tasarım (0 obje) tespiti — IsEmpty() doğru
//   T3  Describe: eski tek formlu v2 biçimi — geriye dönük uyum, 1 form
//   T4  Describe: bozuk/tanınmayan dosya → ok=false (çökmez)
//   T5  DescribeLine: liste satırları (istatistikli / BOŞ / okunamadı)
//   T6  RememberForProject + LastForProject gidiş-dönüş
//   T7  LastForProject: işaret dosyası kurcalanırsa (yol enjeksiyonu) → reddet
//   T8  LastForProject: işaret yoksa → boş (diyalog akışı devreye girer)
#include "app.h"
#include "designmem.h"
#include <cstdio>

static int g_pass = 0, g_fail = 0;
static void Chk(bool cond, const wchar_t* name) {
    if (cond) { g_pass++; wprintf(L"  [PASS] %ls\n", name); }
    else      { g_fail++; wprintf(L"  [FAIL] %ls\n", name); }
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    // geçici test klasörü (exe yanında _t16tmp)
    std::wstring dir = L"_t16tmp";
    CreateDirectoryW(dir.c_str(), nullptr);
    DeleteFileW((dir + L"\\.m2god_last").c_str());

    // --- T1: dolu v3 proje (2 form, 3+0 obje, Modern) ------------------------
    std::wstring full = dir + L"\\Full.m2god";
    WriteFileText(full,
        "{\n"
        "  \"format\": \"m2god-project\",\n"
        "  \"version\": 3.0,\n"
        "  \"active\": 0,\n"
        "  \"Theme\": \"Modern\",\n"
        "  \"forms\": [\n"
        "    { \"name\": \"Form1\", \"design\": { \"format\": \"m2god-form\", "
        "\"version\": 2.0, \"Form\": {}, \"Controls\": [ {}, {}, {} ] } },\n"
        "    { \"name\": \"Form2\", \"design\": { \"format\": \"m2god-form\", "
        "\"version\": 2.0, \"Form\": {}, \"Controls\": [] } }\n"
        "  ]\n"
        "}");
    designmem::DesignStats st = designmem::Describe(full);
    Chk(st.ok && st.forms == 2 && st.objects == 3 && st.theme == L"Modern",
        L"T1 dolu v3 proje: 2 form + 3 obje + Modern teması");

    // --- T2: BOŞ tasarım (kullanıcının 2.12/2.13 çöpü) -----------------------
    std::wstring empty = dir + L"\\Empty.m2god";
    WriteFileText(empty,
        "{ \"format\": \"m2god-project\", \"version\": 3.0, \"active\": 0,"
        "  \"forms\": [ { \"name\": \"Form1\", \"design\": { \"Controls\": [] } } ] }");
    designmem::DesignStats st2 = designmem::Describe(empty);
    Chk(st2.ok && st2.objects == 0 && st2.IsEmpty(),
        L"T2 boş tasarım tespiti: 0 obje → IsEmpty (çöp etiketi)");

    // --- T3: eski tek formlu v2 biçimi ----------------------------------------
    std::wstring legacy = dir + L"\\Legacy.m2god";
    WriteFileText(legacy,
        "{ \"format\": \"m2god-form\", \"version\": 2.0,"
        "  \"Form\": { \"Name\": \"Form1\" },"
        "  \"Controls\": [ {}, {} ] }");
    designmem::DesignStats st3 = designmem::Describe(legacy);
    Chk(st3.ok && st3.forms == 1 && st3.objects == 2,
        L"T3 eski v2 tek formlu biçim: geriye dönük uyum (1 form, 2 obje)");

    // --- T4: bozuk dosya -------------------------------------------------------
    std::wstring junk = dir + L"\\Junk.m2god";
    WriteFileText(junk, "bu bir json degil {{{");
    designmem::DesignStats st4 = designmem::Describe(junk);
    Chk(!st4.ok, L"T4 bozuk dosya: ok=false (çökmez, '(okunamadı)' etiketi)");

    // --- T5: liste satırları ---------------------------------------------------
    Chk(designmem::DescribeLine(L"Full.m2god", st).find(L"3 obje") != std::wstring::npos,
        L"T5a istatistik satırı: '... — 2 form, 3 obje, Modern'");
    Chk(designmem::DescribeLine(L"Empty.m2god", st2).find(L"BOŞ") != std::wstring::npos,
        L"T5b boş satır: 'BOŞ tasarım (0 obje)' etiketi");
    Chk(designmem::DescribeLine(L"Junk.m2god", st4).find(L"okunamadı") != std::wstring::npos,
        L"T5c okunamadı satırı");

    // --- T6: proje hafızası gidiş-dönüş ---------------------------------------
    Chk(designmem::RememberForProject(dir, full), L"T6a işaret yazma başarılı");
    Chk(designmem::LastForProject(dir) == L"Full.m2god",
        L"T6b hatırlanan tasarım doğru okundu (diyalogsuz otomatik yükleme kaynağı)");

    // --- T7: kurcalanmış işaret (yol enjeksionu) reddi -------------------------
    WriteFileText(dir + L"\\.m2god_last", "..\\\\..\\\\Windows\\\\evil.m2god");
    Chk(designmem::LastForProject(dir).empty(),
        L"T7 işaret kurcalandı (yol içeriyor) → red: diyalog güvenli devreye girer");

    // --- T8: işaret yok ---------------------------------------------------------
    DeleteFileW((dir + L"\\.m2god_last").c_str());
    Chk(designmem::LastForProject(dir).empty(),
        L"T8 işaret yok → boş (ilk bağlanışta seçim diyalogu normal davranır)");

    // temizlik
    DeleteFileW(full.c_str()); DeleteFileW(empty.c_str());
    DeleteFileW(legacy.c_str()); DeleteFileW(junk.c_str());
    RemoveDirectoryW(dir.c_str());

    wprintf(L"\nSONUC: %d PASS / %d FAIL\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
