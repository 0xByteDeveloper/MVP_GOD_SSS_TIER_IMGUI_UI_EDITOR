// designmem.cpp — FAZ 2.16: tasarım hafızası gerçeklemesi (bkz. designmem.h)
#include "designmem.h"
#include "json.h"
#include "app.h"          // ReadFileText / WriteFileText / GetFileName
#include <shlobj.h>       // SHGetFolderPathW (APPDATA)
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif

namespace designmem {

// --- istatistik: .m2god hafif JSON taraması ------------------------------------
DesignStats Describe(const std::wstring& path) {
    DesignStats st;
    std::string data;
    if (!ReadFileText(path, data)) return st;
    bool ok = false;
    json::Value root = json::Parse(data, &ok);
    if (!ok || !root.IsObj()) return st;

    auto ctrlCount = [](const json::Value& design) -> int {
        const json::Value& ctrls = design.Get(L"Controls");
        return ctrls.IsArr() ? (int)ctrls.arr.size() : 0;
    };

    if (root.Get(L"format").AsStr() == L"m2god-project" && root.Get(L"forms").IsArr()) {
        // Faz 2.7+ çok formlu proje dosyası (version 3)
        st.theme = root.Get(L"Theme").AsStr();
        const json::Value& forms = root.Get(L"forms");
        int total = 0;
        for (auto& f : forms.arr) {
            const json::Value& d = f.Get(L"design");
            if (d.IsObj()) {
                st.forms++;
                total += ctrlCount(d);
            }
        }
        st.objects = total;
    } else if (root.IsObj() && root.Get(L"Controls").IsArr()) {
        // eski tek formlu biçim (m2god-form v2 / Faz 1-6 kök tasarım JSON'u)
        st.forms = 1;
        st.objects = ctrlCount(root);
    } else {
        return st;   // tanınmayan biçim → ok=false
    }
    st.ok = true;
    return st;
}

std::wstring DescribeLine(const std::wstring& fileName, const DesignStats& st) {
    std::wstring line = fileName;
    if (!st.ok)            line += L" — (okunamadı)";
    else if (st.IsEmpty()) line += L" — BOŞ tasarım (0 obje) — çöp";
    else {
        line += L" — " + std::to_wstring(st.forms) + L" form, " +
                std::to_wstring(st.objects) + L" obje";
        if (!st.theme.empty()) line += L", " + st.theme;
    }
    return line;
}

// --- 1) proje hafızası: <projDir>\.m2god_last ----------------------------------
static std::wstring PtrPath(const std::wstring& projDir) {
    return projDir + L"\\.m2god_last";
}

bool RememberForProject(const std::wstring& projDir, const std::wstring& designFile) {
    if (projDir.empty()) return false;
    // SADECE dosya adı saklanır — içinde ayraç geçen değer reddedilir
    // (işaret dosyası elle kurcalanıp yol enjeksiyonu yapılmasın)
    std::wstring name = GetFileName(designFile);
    if (name.empty() || name.find(L'\\') != std::wstring::npos ||
        name.find(L'/')  != std::wstring::npos)
        return false;
    return WriteFileText(PtrPath(projDir), WideToUtf8(name));
}

std::wstring LastForProject(const std::wstring& projDir) {
    if (projDir.empty()) return L"";
    std::string data;
    if (!ReadFileText(PtrPath(projDir), data)) return L"";
    std::wstring name = Trim(Utf8ToWide(data));
    // güvenlik: ayraç içeren değer çöptür (başka klasörü hedefleemesin)
    if (name.find(L'\\') != std::wstring::npos ||
        name.find(L'/')  != std::wstring::npos ||
        name.find(L'.')  == 0)   // ".m2god_last" kendisi, gizli dosya vb.
        return L"";
    return name;
}

// --- 2) oturum hafızası: %APPDATA%\M2GodWin32\last_session.txt -----------------
static std::wstring SessionDir() {
    wchar_t buf[MAX_PATH] = L"";
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                nullptr, 0, buf)))
        return L"";
    std::wstring dir = std::wstring(buf) + L"\\M2GodWin32";
    CreateDirectoryW(dir.c_str(), nullptr);   // varsa sessizce başarısız olur — sorun değil
    return dir;
}

bool RememberSession(const std::wstring& designFile) {
    if (designFile.empty()) return false;
    std::wstring dir = SessionDir();
    if (dir.empty()) return false;
    return WriteFileText(dir + L"\\last_session.txt", WideToUtf8(designFile));
}

std::wstring LastSession() {
    std::wstring dir = SessionDir();
    if (dir.empty()) return L"";
    std::string data;
    if (!ReadFileText(dir + L"\\last_session.txt", data)) return L"";
    std::wstring p = Trim(Utf8ToWide(data));
    return FileExists(p) ? p : L"";
}

} // namespace designmem
