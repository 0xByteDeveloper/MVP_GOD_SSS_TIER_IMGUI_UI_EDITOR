// json.h — minimal JSON ayrıştırıcı/yazıcı (UTF-8), Config_info.json ve proje
// kaydetme/yükleme için yeterli. Bağımlılıksız.
#pragma once
#include <string>
#include <vector>
#include <cwctype>

// (imgod.cpp'de uygulanan utf-8 ↔ utf-16 dönüşümleri — bildirim)
std::wstring Utf8ToWideStr(const std::string& s);
std::string  WideStrToUtf8(const std::wstring& w);

namespace json {

class Value {
public:
    enum Type { NUL, BOOL, NUM, STR, ARR, OBJ };
    Type t = NUL;
    bool b = false;
    double num = 0;
    std::wstring str;
    std::vector<Value> arr;
    std::vector<std::pair<std::wstring, Value>> obj;

    Value() {}
    static Value MakeStr(const std::wstring& s) { Value v; v.t = STR; v.str = s; return v; }
    static Value MakeNum(double d) { Value v; v.t = NUM; v.num = d; return v; }
    static Value MakeBool(bool b) { Value v; v.t = BOOL; v.b = b; return v; }
    static Value MakeArr() { Value v; v.t = ARR; return v; }
    static Value MakeObj() { Value v; v.t = OBJ; return v; }

    bool IsObj() const { return t == OBJ; }
    bool IsArr() const { return t == ARR; }
    bool IsStr() const { return t == STR; }
    bool IsNum() const { return t == NUM; }

    // Nesne alanı erişimi (yoksa NUL döner)
    const Value& Get(const wchar_t* key) const {
        if (t == OBJ)
            for (auto& kv : obj) if (kv.first == key) return kv.second;
        static Value nul;
        return nul;
    }
    bool Has(const wchar_t* key) const {
        if (t == OBJ) for (auto& kv : obj) if (kv.first == key) return true;
        return false;
    }
    void Set(const std::wstring& key, Value v) {
        if (t != OBJ) { t = OBJ; obj.clear(); }
        for (auto& kv : obj) if (kv.first == key) { kv.second = v; return; }
        obj.push_back({ key, v });
    }
    void Set(const std::wstring& key, const std::wstring& s) { Set(key, MakeStr(s)); }
    void Set(const std::wstring& key, const wchar_t* s) { Set(key, MakeStr(s)); }
    void Set(const std::wstring& key, double d) { Set(key, MakeNum(d)); }
    void Set(const std::wstring& key, bool b) { Set(key, MakeBool(b)); }
    // dar (utf-8) anahtarlı aşırı yüklemeler — otomatik geniş dönüşüm
    void Set(const std::string& key, Value v) { Set(Utf8ToWideStr(key), v); }
    void Set(const std::string& key, const std::string& s) { Set(Utf8ToWideStr(key), MakeStr(Utf8ToWideStr(s))); }
    void Set(const std::string& key, const char* s) { Set(Utf8ToWideStr(key), MakeStr(Utf8ToWideStr(std::string(s)))); }
    void Set(const std::string& key, double d) { Set(Utf8ToWideStr(key), MakeNum(d)); }
    void Set(const std::string& key, bool b) { Set(Utf8ToWideStr(key), MakeBool(b)); }
    void Set(const char* key, Value v) { Set(Utf8ToWideStr(std::string(key)), v); }
    void Set(const char* key, const std::string& s) { Set(Utf8ToWideStr(std::string(key)), MakeStr(Utf8ToWideStr(s))); }
    void Set(const char* key, const char* s) { Set(Utf8ToWideStr(std::string(key)), MakeStr(Utf8ToWideStr(std::string(s)))); }
    void Set(const char* key, double d) { Set(Utf8ToWideStr(std::string(key)), MakeNum(d)); }
    void Set(const char* key, bool b) { Set(Utf8ToWideStr(std::string(key)), MakeBool(b)); }
    void Push(Value v) { if (t != ARR) { t = ARR; arr.clear(); } arr.push_back(v); }
    void Push(const std::string& s) { Push(MakeStr(Utf8ToWideStr(s))); }

    // string/bool/num dönüşümleri
    std::wstring AsStr(const std::wstring& def = L"") const {
        if (t == STR) return str;
        if (t == NUM) { wchar_t buf[64]; swprintf(buf, 64, L"%g", num); return buf; }
        if (t == BOOL) return b ? L"True" : L"False";
        return def;
    }
    double AsNum(double def = 0) const {
        if (t == NUM) return num;
        if (t == STR) { wchar_t* e = nullptr; double d = wcstod(str.c_str(), &e); if (e != str.c_str()) return d; }
        if (t == BOOL) return b ? 1 : 0;
        return def;
    }
    bool AsBool(bool def = false) const {
        if (t == BOOL) return b;
        if (t == STR) {
            if (str.size() == 4 &&
                towlower((wint_t)str[0]) == L't' && towlower((wint_t)str[1]) == L'r' &&
                towlower((wint_t)str[2]) == L'u' && towlower((wint_t)str[3]) == L'e') return true;
            return str == L"1";
        }
        if (t == NUM) return num != 0;
        return def;
    }
};

// utf-8 girdi; hata durumunda *ok=false
Value Parse(const std::string& utf8, bool* ok = nullptr);
// pretty=true girintili çıktı
std::string Stringify(const Value& v, bool pretty = true);

// —— dar string (utf-8) yardımcıları — dar anahtarlı kısa yollar ——
inline void SetS(Value& v, const std::string& key, const std::string& s) {
    Value sv; sv.t = Value::STR; sv.str = Utf8ToWideStr(s);
    v.Set(Utf8ToWideStr(key), sv);
}
inline void SetN(Value& v, const std::string& key, double d) {
    v.Set(Utf8ToWideStr(key), d);
}
inline void SetB(Value& v, const std::string& key, bool b) {
    v.Set(Utf8ToWideStr(key), b);
}
inline void PushS(Value& v, const std::string& s) {
    Value sv; sv.t = Value::STR; sv.str = Utf8ToWideStr(s);
    v.Push(sv);
}

} // namespace json
