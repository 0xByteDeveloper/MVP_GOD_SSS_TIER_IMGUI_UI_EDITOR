// vsint.cpp — VS entegrasyonu: Windows'ta Toolhelp32 taraması, taşınabilir disk taraması
#include "vsint.h"
#include "imgod.h"
#include <set>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#endif

namespace vsint {

static bool g_connected = false;
static std::string g_sln, g_projDir;

#ifdef _WIN32
std::vector<VSInstance> Enumerate() {
    std::vector<VSInstance> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring exe(pe.szExeFile);
            if (exe == L"devenv.exe") {
                VSInstance v;
                v.pid = pe.th32ProcessID;
                // ana pencereyi bul (EnumWindows — süreç kimliğine göre)
                struct FW { DWORD pid; char title[512]; bool found; } fw{ pe.th32ProcessID, "", false };
                EnumWindows([](HWND h, LPARAM lp) -> BOOL {
                    FW* fw = (FW*)lp;
                    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
                    if (pid == fw->pid && IsWindowVisible(h) && GetWindowTextLengthW(h) > 0) {
                        wchar_t buf[512]; GetWindowTextW(h, buf, 512);
                        size_t n = 0;
                        while (n < 511 && buf[n]) { fw->title[n] = (char)(buf[n] < 128 ? buf[n] : '?'); n++; }
                        fw->title[n] = 0;
                        fw->found = true;
                        return FALSE;   // ilk görünür pencere yeterli
                    }
                    return TRUE;
                }, (LPARAM)&fw);
                v.title = fw.title;
                // başlıktan çözüm adı ayıkla: "MySolution - Microsoft Visual Studio"
                if (!v.title.empty()) {
                    size_t dash = v.title.find(" - Microsoft Visual Studio");
                    if (dash == std::string::npos) dash = v.title.find(" - Visual Studio");
                    if (dash != std::string::npos) v.solutionName = v.title.substr(0, dash);
                    size_t par = v.title.rfind('(');
                    if (par != std::string::npos && v.title.find("Visual Studio") != std::string::npos) {
                        std::string ver = v.title.substr(par + 1);
                        size_t close = ver.find(')');
                        if (close != std::string::npos) v.version = ver.substr(0, close);
                    }
                }
                if (!v.solutionName.empty()) v.solutionPath = FindSlnOnDisk(v.solutionName);
                out.push_back(v);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}
#else
std::vector<VSInstance> Enumerate() {
    // Wine-öncesi Unix/Wine ortamında çalışan devenv yok — boş liste, VS'siz akış tam işler
    return {};
}
#endif

std::vector<std::string> FindSlnsUnder(const std::string& dir, int depth) {
    std::vector<std::string> out;
    if (depth < 0 || dir.empty() || !FileExistsA(dir)) return out;
    // direkt .sln'ler
    std::vector<std::string> files = ListFilesA(dir, ".sln");
    for (auto& f : files) out.push_back(f);
    // alt klasörler (bin/obj/.vs/node_modules atla)
    std::vector<std::string> subs = ListFilesA(dir, "");
    (void)subs;
    // dizin listesi için ayrı tarama:
#ifdef _WIN32
    WIN32_FIND_DATAA fd; HANDLE h;
    std::string pat = dir + "\\*";
    h = FindFirstFileA(pat.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::string n(fd.cFileName);
                if (n == "." || n == ".." || n == "bin" || n == "obj" || n == ".vs" || n == "node_modules") continue;
                auto sub = FindSlnsUnder(dir + "\\" + n, depth - 1);
                for (auto& s : sub) out.push_back(s);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != nullptr) {
            std::string n(e->d_name);
            if (n == "." || n == ".." || n == "bin" || n == "obj" || n == ".vs" || n == "node_modules") continue;
            std::string full = JoinPathA(dir, n);
            struct stat st;
            if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                auto sub = FindSlnsUnder(full, depth - 1);
                for (auto& s : sub) out.push_back(s);
            }
        }
        closedir(d);
    }
#endif
    // tekrarları ayıkla
    std::set<std::string> seen;
    std::vector<std::string> uniq;
    for (auto& s : out) if (seen.insert(s).second) uniq.push_back(s);
    return uniq;
}

std::string FindSlnOnDisk(const std::string& solutionName) {
    // bilinen kökler — Windows kullanıcı klasörleri
    const char* roots[] = {
        "C:\\Users\\Public\\Documents", "C:\\source", "C:\\src", "C:\\repos",
        "D:\\source", "D:\\repos",
    };
    for (const char* r : roots) {
        if (!FileExistsA(r)) continue;
        auto slns = FindSlnsUnder(r, 3);
        for (auto& s : slns) {
            std::string base = BaseNameA(s);
            if (base.size() > 4 && base.substr(0, base.size() - 4) == solutionName) return s;
        }
    }
    return "";
}

bool Connect(const std::string& solutionPath, const std::string& projectDir) {
    if (!FileExistsA(solutionPath)) return false;
    g_sln = solutionPath;
    g_projDir = projectDir.empty() ? DirOfA(solutionPath) : projectDir;
    g_connected = true;
    return true;
}
void Disconnect() { g_connected = false; g_sln.clear(); g_projDir.clear(); }
bool IsConnected() { return g_connected; }
std::string ConnectedSln() { return g_sln; }
std::string ProjectDir() { return g_projDir; }

bool OpenSolution(const std::string& slnPath) {
#ifdef _WIN32
    if (!FileExistsA(slnPath)) return false;
    HINSTANCE h = ShellExecuteA(nullptr, "open", slnPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return (INT_PTR)h > 32;
#else
    (void)slnPath;
    return false;   // Wine altında VS yok — zarif başarısızlık
#endif
}

} // namespace vsint
