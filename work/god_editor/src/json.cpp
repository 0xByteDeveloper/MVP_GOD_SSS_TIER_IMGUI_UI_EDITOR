// json.cpp — JSON parse/stringify (taşınabilir UTF-8 ↔ UTF-16 dönüşümü)
#include "json.h"
#include <string>
#include <vector>

namespace json {

// —— taşınabilir UTF dönüşümleri (Windows CRT bağımlılığı YOK) ——
static std::wstring Utf8ToWide(const std::string& s) {
    std::wstring out; out.reserve(s.size());
    size_t i = 0; unsigned cp = 0; int rem = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        if (rem == 0) {
            if (c < 0x80) cp = c;
            else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; rem = 1; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; rem = 2; }
            else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; rem = 3; }
            else { cp = 0xFFFD; }
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
static std::string WideToUtf8(const std::wstring& w) {
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

struct Parser {
    const std::wstring& s;
    size_t i = 0;
    bool fail = false;

    explicit Parser(const std::wstring& src) : s(src) {}

    void SkipWs() { while (i < s.size() && (s[i] == L' ' || s[i] == L'\t' || s[i] == L'\r' || s[i] == L'\n')) i++; }
    wchar_t Peek() { SkipWs(); return i < s.size() ? s[i] : 0; }
    bool Eat(wchar_t c) { if (Peek() == c) { i++; return true; } return false; }
    void Expect(wchar_t c) { if (!Eat(c)) fail = true; }

    Value ParseValue() {
        Value v;
        if (fail || i >= s.size()) { fail = true; return v; }
        wchar_t c = Peek();
        if (c == L'{') return ParseObj();
        if (c == L'[') return ParseArr();
        if (c == L'"') { v.t = Value::STR; v.str = ParseStr(); return v; }
        if (c == L't') { Lit(L"true"); v.t = Value::BOOL; v.b = true; return v; }
        if (c == L'f') { Lit(L"false"); v.t = Value::BOOL; v.b = false; return v; }
        if (c == L'n') { Lit(L"null"); return v; }
        return ParseNum();
    }
    void Lit(const wchar_t* lit) {
        SkipWs();
        size_t n = wcslen(lit);
        if (s.compare(i, n, lit) == 0) i += n; else fail = true;
    }
    Value ParseNum() {
        SkipWs();
        size_t st = i;
        while (i < s.size() && (iswdigit(s[i]) || s[i] == L'-' || s[i] == L'+' || s[i] == L'.' || s[i] == L'e' || s[i] == L'E')) i++;
        Value v; v.t = Value::NUM;
        if (i == st) { fail = true; return v; }
        v.num = wcstod(s.substr(st, i - st).c_str(), nullptr);
        return v;
    }
    std::wstring ParseStr() {
        std::wstring out;
        Expect(L'"');
        if (fail) return out;
        while (i < s.size()) {
            wchar_t c = s[i++];
            if (c == L'"') return out;
            if (c == L'\\') {
                if (i >= s.size()) break;
                wchar_t e = s[i++];
                switch (e) {
                case L'"': out += L'"'; break;
                case L'\\': out += L'\\'; break;
                case L'/': out += L'/'; break;
                case L'b': out += L'\b'; break;
                case L'f': out += L'\f'; break;
                case L'n': out += L'\n'; break;
                case L'r': out += L'\r'; break;
                case L't': out += L'\t'; break;
                case L'u': {
                    unsigned cp = Hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < s.size() && s[i] == L'\\' && s[i + 1] == L'u') {
                        i += 2;
                        unsigned lo = Hex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    out += Utf16(cp);
                    break;
                }
                default: fail = true; return out;
                }
            } else out += c;
        }
        fail = true;
        return out;
    }
    static wchar_t Utf16(unsigned cp) {
        // cp bmp içindese direkt; (surrogate zaten birleştirildi)
        return (wchar_t)cp; // hedef wchar_t=16bit (Windows); bmp dışı naif ama yeterli
    }
    unsigned Hex4() {
        unsigned v = 0;
        for (int k = 0; k < 4 && i < s.size(); k++) {
            wchar_t c = s[i++];
            v <<= 4;
            if (c >= L'0' && c <= L'9') v |= c - L'0';
            else if (c >= L'a' && c <= L'f') v |= c - L'a' + 10;
            else if (c >= L'A' && c <= L'F') v |= c - L'A' + 10;
            else { fail = true; return 0; }
        }
        return v;
    }
    Value ParseObj() {
        Value v; v.t = Value::OBJ;
        Expect(L'{');
        if (fail) return v;
        if (Eat(L'}')) return v;
        while (!fail) {
            SkipWs();
            if (Peek() != L'"') { fail = true; break; }
            std::wstring key = ParseStr();
            Expect(L':');
            Value val = ParseValue();
            v.obj.push_back({ key, val });
            if (Eat(L',')) continue;
            Expect(L'}');
            break;
        }
        return v;
    }
    Value ParseArr() {
        Value v; v.t = Value::ARR;
        Expect(L'[');
        if (fail) return v;
        if (Eat(L']')) return v;
        while (!fail) {
            v.arr.push_back(ParseValue());
            if (Eat(L',')) continue;
            Expect(L']');
            break;
        }
        return v;
    }
};

static void AppendEscaped(std::string& out, const std::wstring& w) {
    for (wchar_t c : w) {
        switch (c) {
        case L'"':  out += "\\\""; break;
        case L'\\': out += "\\\\"; break;
        case L'\n': out += "\\n"; break;
        case L'\r': out += "\\r"; break;
        case L'\t': out += "\\t"; break;
        default:
            if ((unsigned)c < 0x20) {
                char buf[16]; snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                // utf-8 kodla
                unsigned cp = (unsigned)c;
                if (cp < 0x80) out += (char)cp;
                else if (cp < 0x800) {
                    out += (char)(0xC0 | (cp >> 6));
                    out += (char)(0x80 | (cp & 0x3F));
                } else {
                    out += (char)(0xE0 | (cp >> 12));
                    out += (char)(0x80 | ((cp >> 6) & 0x3F));
                    out += (char)(0x80 | (cp & 0x3F));
                }
            }
        }
    }
}

static void StringifyRec(const Value& v, std::string& out, bool pretty, int depth) {
    auto indent = [&](int d) { if (pretty) { out += "\n"; for (int k = 0; k < d; k++) out += "  "; } };
    switch (v.t) {
    case Value::NUL: out += "null"; break;
    case Value::BOOL: out += v.b ? "true" : "false"; break;
    case Value::NUM: {
        char buf[64];
        if (v.num == (long long)v.num) snprintf(buf, sizeof(buf), "%lld", (long long)v.num);
        else snprintf(buf, sizeof(buf), "%g", v.num);
        out += buf; break;
    }
    case Value::STR: out += '"'; AppendEscaped(out, v.str); out += '"'; break;
    case Value::ARR: {
        out += '[';
        for (size_t k = 0; k < v.arr.size(); k++) {
            if (k) out += ",";
            indent(depth + 1);
            StringifyRec(v.arr[k], out, pretty, depth + 1);
        }
        indent(depth);
        out += ']';
        break;
    }
    case Value::OBJ: {
        out += '{';
        bool first = true;
        for (auto& kv : v.obj) {
            if (!first) out += ",";
            first = false;
            indent(depth + 1);
            out += '"';
            AppendEscaped(out, kv.first);
            out += pretty ? "\": " : "\":";
            StringifyRec(kv.second, out, pretty, depth + 1);
        }
        indent(depth);
        out += '}';
        break;
    }
    }
}

Value Parse(const std::string& utf8, bool* ok) {
    // utf-8 → utf-16 (BOM atılır)
    std::wstring w = Utf8ToWide(utf8);
    if (!w.empty() && w[0] == 0xFEFF) { std::wstring w2(w.begin() + 1, w.end()); w.swap(w2); }
    Parser p(w);
    Value v = p.ParseValue();
    if (ok) *ok = !p.fail;
    return v;
}

std::string Stringify(const Value& v, bool pretty) {
    std::string out;
    StringifyRec(v, out, pretty, 0);
    return out;
}

} // namespace json
