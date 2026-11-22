// imgod.cpp — ortak yardımcıların uygulanışı
#include "imgod.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>

std::wstring Utf8ToWideStr(const std::string& s) {
    std::wstring out; out.reserve(s.size());
    size_t i = 0; unsigned cp = 0; int rem = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (rem == 0) {
            if (c < 0x80) cp = c;
            else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; rem = 1; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; rem = 2; }
            else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; rem = 3; }
            else cp = 0xFFFD;
        } else {
            if ((c & 0xC0) == 0x80) { cp = (cp << 6) | (c & 0x3F); rem--; }
            else { cp = 0xFFFD; rem = 0; }
        }
        i++;
        if (rem == 0) {
            if (cp > 0x10FFFF) cp = 0xFFFD;
            if (cp >= 0xD800 && cp <= 0xDFFF) cp = 0xFFFD;
            if (cp < 0x10000) out += (wchar_t)cp;
            else { cp -= 0x10000; out += (wchar_t)(0xD800 + (cp >> 10)); out += (wchar_t)(0xDC00 + (cp & 0x3FF)); }
        }
    }
    return out;
}
std::string WideStrToUtf8(const std::wstring& w) {
    std::string out; out.reserve(w.size());
    for (size_t i = 0; i < w.size(); i++) {
        unsigned cp = (unsigned)w[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < w.size() &&
            w[i+1] >= 0xDC00 && w[i+1] <= 0xDFFF) {
            cp = 0x10000 + ((cp - 0xD800) << 10) + ((unsigned)w[i+1] - 0xDC00); i++;
        }
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
        else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
    }
    return out;
}
std::wstring TrimW(const std::wstring& s) {
    size_t a = 0, b = s.size();
    while (a < b && iswspace(s[a])) a++;
    while (b > a && iswspace(s[b-1])) b--;
    return s.substr(a, b - a);
}
std::wstring LowerW(std::wstring s) { for (auto& c : s) c = (wchar_t)towlower(c); return s; }
bool EqCIW(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (towlower(a[i]) != towlower(b[i])) return false;
    return true;
}
std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out; std::wstring cur;
    for (wchar_t c : s) { if (c == sep) { out.push_back(cur); cur.clear(); } else cur += c; }
    out.push_back(cur);
    return out;
}
std::wstring JoinW(const std::vector<std::wstring>& v, wchar_t sep) {
    std::wstring out;
    for (size_t i = 0; i < v.size(); i++) { if (i) out += sep; out += v[i]; }
    return out;
}
std::wstring ReplaceAllW(std::wstring s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}
std::string TrimA(std::string s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b - a);
}

bool ReadFileTextA(const std::string& path, std::string& out) {
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf();
    out = ss.str();
    return true;
}
bool WriteFileTextA(const std::string& path, const std::string& data) {
    std::ofstream f(path.c_str(), std::ios::binary);
    if (!f) return false;
    f.write(data.data(), (std::streamsize)data.size());
    return f.good();
}
bool FileExistsA(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}
bool MkDirsA(const std::string& path) {
    if (path.empty()) return false;
    if (FileExistsA(path)) return true;
    std::string cur;
    for (size_t i = 0; i < path.size(); i++) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            if (!cur.empty() && !FileExistsA(cur)) {
#ifdef _WIN32
                if (mkdir(cur.c_str()) != 0 && errno != EEXIST) return false;
#else
                if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) return false;
#endif
            }
        }
        cur += c;
    }
    if (!cur.empty() && !FileExistsA(cur)) {
#ifdef _WIN32
        if (mkdir(cur.c_str()) != 0) return false;
#else
        if (mkdir(cur.c_str(), 0755) != 0) return false;
#endif
    }
    return true;
}
std::string DirOfA(const std::string& path) {
    size_t p = path.find_last_of("/\\");
    if (p == std::string::npos) return "";
    return path.substr(0, p);
}
std::string JoinPathA(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char last = a[a.size()-1];
    if (last == '/' || last == '\\') return a + b;
    return a + "/" + b;
}
std::string BaseNameA(const std::string& path) {
    size_t p = path.find_last_of("/\\");
    return p == std::string::npos ? path : path.substr(p + 1);
}
std::vector<std::string> ListFilesA(const std::string& dir, const std::string& ext) {
    std::vector<std::string> out;
    DIR* d = opendir(dir.c_str());
    if (!d) return out;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
        std::string n(e->d_name);
        if (n == "." || n == "..") continue;
        if (!ext.empty()) {
            if (n.size() <= ext.size()) continue;
            std::string low = n.substr(n.size() - ext.size());
            for (auto& c : low) c = (char)tolower(c);
            if (low != ext) continue;
        }
        out.push_back(JoinPathA(dir, n));
    }
    closedir(d);
    return out;
}

std::string EscCppUtf8(const std::string& s) {
    std::string e;
    for (unsigned char c : s) {
        switch (c) {
        case '\\': e += "\\\\"; break;
        case '"':  e += "\\\""; break;
        case '\n': e += "\\n"; break;
        case '\r': e += "\\r"; break;
        case '\t': e += "\\t"; break;
        default:
            if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\x%02x", c); e += b; }
            else e += (char)c;   // utf-8 baytları aynen (u8"" içinde güvenli)
        }
    }
    return e;
}
std::string EscIdent(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        bool okd = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
        out += okd ? (char)c : '_';
    }
    if (out.empty() || !(out[0] == '_' || (out[0] >= 'A' && out[0] <= 'Z') || (out[0] >= 'a' && out[0] <= 'z'))) out = "_" + out;
    return out;
}

const char* BackendNameA(EditorBackend b) {
    switch (b) {
    case EditorBackend::DX11: return "DirectX 11";
    case EditorBackend::DX9:  return "DirectX 9";
    case EditorBackend::DX10: return "DirectX 10";
    case EditorBackend::DX12: return "DirectX 12";
    case EditorBackend::OPENGL3: return "OpenGL 3";
    }
    return "?";
}
const char* BackendIdA(EditorBackend b) {
    switch (b) {
    case EditorBackend::DX11: return "dx11";
    case EditorBackend::DX9:  return "dx9";
    case EditorBackend::DX10: return "dx10";
    case EditorBackend::DX12: return "dx12";
    case EditorBackend::OPENGL3: return "opengl3";
    }
    return "?";
}

bool g_modelDirty = false;
bool g_gridVisible = true;
bool g_snapEnabled = true;
bool g_hideLabels = false;   // 6. TUR: yan etiketler varsayılan GÖRÜNÜR (M2God/VS paritesi);
                            // Görünüm menüsünden veya araç çubuğundan kapatılabilir
std::string g_statusText;
std::string g_exeDirA;

void* g_mainHwnd = nullptr;
