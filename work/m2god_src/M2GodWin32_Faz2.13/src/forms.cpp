// forms.cpp — FAZ 2.7: çok formlu proje katmanı gerçeklemesi
#include "forms.h"
#include "app.h"
#include "model.h"
#include "json.h"
#include "theme.h"   // FAZ 2.10: proje teması ad↔kimlik
#include <cwctype>
#include <deque>

namespace forms {

struct FormDoc {
    std::wstring name;                 // "Form1", "Form2"...
    json::Value  design;               // Model::ToJson anlık durumu (park edilmiş)
    std::deque<std::string> undo, redo; // forma ÖZEL Geri Al/Yinele yığınları
};
static std::vector<FormDoc> g_docs;
static int g_active = 0;
static int g_themeId = 0;   // FAZ 2.10: proje varsayılan teması (0=Oldschool)

// --- iç yardımıcılar -----------------------------------------------------------

// aktif modeli belgeye park et (anlık durum + yığınlar modele kalmaz)
static void ParkActive() {
    if (g_active < 0 || g_active >= (int)g_docs.size()) return;
    g_docs[g_active].design = g_model.ToJson();
    g_docs[g_active].undo = g_model.DetachUndo();
    g_docs[g_active].redo = g_model.DetachRedo();
}

// belgeyi aktif modele geri yükle (FromJson HWND'leri yok eder — çağıran
// taraf surface::RecreateAll ile yeniden kurar)
static void Unpark(int i) {
    if (i < 0 || i >= (int)g_docs.size()) return;
    g_model.FromJson(g_docs[i].design);
    // ad senkronu: tasarımın içindeki Name otorite; boşsa belge adı kalır
    std::wstring nm = g_model.formProps.count(L"Name") ? g_model.formProps[L"Name"] : L"";
    if (!nm.empty()) g_docs[i].name = nm;
    g_model.AttachUndo(g_docs[i].undo, g_docs[i].redo);
    g_active = i;
}

// boş form modeli (InitNew / AddNew için)
static void BlankModel(const std::wstring& nm) {
    g_model.ClearAll();
    g_model.formProps = { { L"Text", nm }, { L"Name", nm }, { L"BackColor", L"Control" } };
    g_model.formW = 800; g_model.formH = 600;
    g_model.selected = nullptr;
    g_model.formSelected = true;
    g_model.ClearUndo();
}

// --- API -----------------------------------------------------------------------

void InitNew() {
    g_docs.clear();
    FormDoc d; d.name = L"Form1";
    g_docs.push_back(d);
    g_active = 0;
    BlankModel(L"Form1");
}

int Count() { return (int)g_docs.size(); }
int ActiveIndex() { return g_active; }

std::wstring ActiveName() {
    if (g_active >= 0 && g_active < (int)g_docs.size()) return g_docs[g_active].name;
    return L"Form1";
}

std::vector<std::wstring> Names() {
    std::vector<std::wstring> out;
    for (auto& d : g_docs) out.push_back(d.name);
    return out;
}

std::wstring NextFreeName() {
    for (int n = 1; n < 1000; n++) {
        std::wstring cand = L"Form" + std::to_wstring(n);
        bool clash = false;
        for (auto& d : g_docs)
            if (_wcsicmp(d.name.c_str(), cand.c_str()) == 0) { clash = true; break; }
        if (!clash) return cand;
    }
    return L"Form_x";
}

std::wstring AddNew() {
    std::wstring nm = NextFreeName();
    ParkActive();
    FormDoc d; d.name = nm;
    g_docs.push_back(d);
    int idx = (int)g_docs.size() - 1;
    BlankModel(nm);
    // belgeye anlık durumu da yaz (kayıt her an çağrılabilir)
    g_docs[idx].design = g_model.ToJson();
    g_active = idx;
    return nm;
}

bool SwitchTo(int index) {
    if (index < 0 || index >= (int)g_docs.size() || index == g_active) return false;
    ParkActive();
    Unpark(index);
    return true;
}

bool ValidName(const std::wstring& n) {
    if (n.empty() || n.size() > 40) return false;
    if (!iswalpha(n[0]) && n[0] != L'_') return false;
    for (wchar_t c : n)
        if (!iswalnum(c) && c != L'_') return false;
    // projede benzersiz (kendisi hariç — aynı ad = değişiklik yok sayılır)
    for (int i = 0; i < (int)g_docs.size(); i++)
        if (i != g_active && _wcsicmp(g_docs[i].name.c_str(), n.c_str()) == 0) return false;
    return true;
}

bool RenameActive(const std::wstring& newName) {
    if (g_active < 0 || g_active >= (int)g_docs.size()) return false;
    if (!ValidName(newName)) return false;
    std::wstring old = g_docs[g_active].name;
    g_docs[g_active].name = newName;
    // model tarafı: Name her zaman; Text başlık olarak eski ada eşitse o da
    if (!g_model.formProps.count(L"Text") || g_model.formProps[L"Text"] == old)
        g_model.formProps[L"Text"] = newName;
    g_model.formProps[L"Name"] = newName;
    // form olay işleyici adları "Form2_Load" öneki taşıyorsa yeni ada taşı
    std::wstring pre = old + L"_";
    for (auto& kv : g_model.formEvents)
        if (_wcsnicmp(kv.second.c_str(), pre.c_str(), pre.size()) == 0)
            kv.second = newName + L"_" + kv.second.substr(pre.size());
    return true;
}

bool RemoveAt(int index) {
    if ((int)g_docs.size() <= 1) return false;              // son form silinmez
    if (index < 0 || index >= (int)g_docs.size()) return false;
    if (index == g_active) {
        int next = (index + 1 < (int)g_docs.size()) ? index + 1 : index - 1;
        ParkActive();
        Unpark(next);
        g_docs.erase(g_docs.begin() + index);
        if (g_active > index) g_active--;
    } else {
        g_docs.erase(g_docs.begin() + index);
        if (g_active > index) g_active--;
    }
    return true;
}

// --- kaydet / yükle -------------------------------------------------------------

bool SaveProject(const std::wstring& path) {
    // aktif formun anlık durumunu snapshot'la (yığınlar modele kalır)
    if (g_active >= 0 && g_active < (int)g_docs.size())
        g_docs[g_active].design = g_model.ToJson();
    json::Value root = json::Value::MakeObj();
    root.Set(L"format", L"m2god-project");
    root.Set(L"version", 3.0);
    root.Set(L"active", (double)g_active);
    root.Set(L"Theme", theme::SaveNameOf(g_themeId));   // FAZ 2.10: Oldschool/Modern/AppleMac
    json::Value arr = json::Value::MakeArr();
    for (auto& d : g_docs) {
        json::Value o = json::Value::MakeObj();
        o.Set(L"name", d.name);
        o.Set(L"design", d.design);
        arr.Push(o);
    }
    root.Set(L"forms", arr);
    return WriteFileText(path, json::Stringify(root, true));
}

bool LoadProject(const std::wstring& path) {
    std::string data;
    if (!ReadFileText(path, data)) return false;
    bool ok = false;
    json::Value root = json::Parse(data, &ok);
    if (!ok) return false;

    std::vector<FormDoc> docs;
    int active = 0;
    if (root.Get(L"format").AsStr() == L"m2god-project" && root.Get(L"forms").IsArr()) {
        // FAZ 2.10: kayıtlı tema (yoksa/eski dosyaysa Oldschool varsayılan)
        std::wstring th = root.Get(L"Theme").AsStr();
        int tid = theme::IdOfName(th.c_str());
        g_themeId = (tid >= 0) ? tid : 0;
        for (auto& f : root.Get(L"forms").arr) {
            FormDoc d;
            d.name = f.Get(L"name").AsStr();
            d.design = f.Get(L"design");
            if (d.name.empty())
                d.name = d.design.Get(L"Form").Get(L"Name").AsStr(L"Form1");
            if (d.name.empty()) d.name = L"Form1";
            docs.push_back(d);
        }
        active = (int)root.Get(L"active").AsNum(0);
    } else {
        // eski tek formlu format (m2god-form / sürüm 2): kök = tasarımın kendisi
        FormDoc d;
        d.design = root;
        d.name = root.Get(L"Form").Get(L"Name").AsStr(L"Form1");
        if (d.name.empty()) d.name = L"Form1";
        docs.push_back(d);
    }
    if (docs.empty()) return false;
    if (active < 0 || active >= (int)docs.size()) active = 0;

    g_docs = docs;
    g_active = -1;          // Unpark her durumda tam yükleme yapsın
    Unpark(active);
    return true;
}

// --- FAZ 2.13: KLASÖR PROJESİ (Forms.info + Form1.info/Form2.info...) ----------
// Kullanıcı isteği: "eskiden Form1.m2god gibi miydi; artık multiple form
// kullanabildiğimiz için Forms.info (içinde formların bilgileri olur kaç form
// olduğu) — kaç form varsa Form1.info, Form2.info gibi olur."
//
// Forms.info : { format:"m2god-folder", version:1.0, Theme:"Ruby",
//                Count:2, Forms:["MainForm","Clientsettings1"], Active:0 }
// FormN.info : o formun tasarımı (name + design — .m2god'daki formlar[i] ile
//              aynı yapı; dosya adı sıralama düzinedir, içerik otoritedir)
bool SaveFolderProject(const std::wstring& dir) {
    if (dir.empty()) return false;
    // aktif formun anlık durumunu park et
    if (g_active >= 0 && g_active < (int)g_docs.size())
        g_docs[g_active].design = g_model.ToJson();
    json::Value root = json::Value::MakeObj();
    root.Set(L"format", L"m2god-folder");
    root.Set(L"version", 1.0);
    root.Set(L"Theme", theme::SaveNameOf(g_themeId));
    root.Set(L"Count", (double)g_docs.size());
    json::Value names = json::Value::MakeArr();
    for (auto& d : g_docs) names.Push(json::Value::MakeStr(d.name));
    root.Set(L"Forms", names);
    root.Set(L"Active", (double)g_active);
    if (!WriteFileText(dir + L"\\Forms.info", json::Stringify(root, true))) return false;
    // her forma bir FormN.info (N = 1..sayı)
    for (size_t i = 0; i < g_docs.size(); i++) {
        json::Value o = json::Value::MakeObj();
        o.Set(L"name", g_docs[i].name);
        o.Set(L"design", g_docs[i].design);
        std::wstring fp = dir + L"\\Form" + std::to_wstring(i + 1) + L".info";
        if (!WriteFileText(fp, json::Stringify(o, true))) return false;
    }
    return true;
}

bool LoadFolderProject(const std::wstring& dir) {
    if (dir.empty()) return false;
    std::string data;
    if (!ReadFileText(dir + L"\\Forms.info", data)) return false;
    bool ok = false;
    json::Value root = json::Parse(data, &ok);
    if (!ok || root.Get(L"format").AsStr() != L"m2god-folder") return false;

    // tema
    std::wstring th = root.Get(L"Theme").AsStr();
    int tid = theme::IdOfName(th.c_str());
    g_themeId = (tid >= 0) ? tid : 0;

    std::vector<FormDoc> docs;
    // formlar: Forms ad listesi → FormN.info'ları SIRAYLA yükle (dosya adı
    // düzeneği — içerikteki name ne olursa olsun sıra korunur; eski kopyalarda
    // dosya eksikse Count/Forms bilgisine güvenip boş form bırakılır)
    int count = (int)root.Get(L"Count").AsNum(0);
    if (count <= 0) {
        const json::Value& arr = root.Get(L"Forms");
        count = arr.IsArr() ? (int)arr.arr.size() : 0;
    }
    if (count <= 0) return false;
    for (int i = 0; i < count; i++) {
        FormDoc d;
        std::wstring fp = dir + L"\\Form" + std::to_wstring(i + 1) + L".info";
        std::string fdata;
        if (ReadFileText(fp, fdata)) {
            bool fok = false;
            json::Value fo = json::Parse(fdata, &fok);
            if (fok) {
                d.design = fo.Get(L"design");
                d.name = fo.Get(L"name").AsStr();
                if (d.name.empty())
                    d.name = d.design.Get(L"Form").Get(L"Name").AsStr(L"Form1");
            }
        }
        // Forms.info'daki ad listesi öncelikli (dizinin otoritesi)
        const json::Value& arr = root.Get(L"Forms");
        if (arr.IsArr() && i < (int)arr.arr.size() && !arr.arr[i].AsStr().empty())
            d.name = arr.arr[i].AsStr();
        if (d.name.empty()) d.name = L"Form" + std::to_wstring(i + 1);
        docs.push_back(d);
    }
    int active = (int)root.Get(L"Active").AsNum(0);
    if (active < 0 || active >= (int)docs.size()) active = 0;
    g_docs = docs;
    g_active = -1;
    Unpark(active);
    return true;
}

// --- FAZ 2.10: proje teması ---------------------------------------------------------

int  ThemeId() { return g_themeId; }
void SetThemeId(int t) { g_themeId = (t >= 0 && t <= 6) ? t : 0; }   // FAZ 2.13: 7 tema (M2God grafit 4'lüsü dahil)

// --- FAZ 2.9: Kod Gezgini veri kaynağı ---------------------------------------------
// Aktif form → g_model (canlı); parklı formlar → belgedeki tasarım JSON'u.
// Böylece AĞAÇTA tüm formlar genişletilebilir: hangi formun objelerini
// görürseniz o form aktif olmasa bile listedir.

std::vector<TreeObj> TreeObjectsOf(int formIdx) {
    std::vector<TreeObj> out;
    if (formIdx < 0 || formIdx >= (int)g_docs.size()) return out;
    if (formIdx == g_active) {                       // canlı model
        for (auto* c : g_model.ctrls) {
            TreeObj t;
            t.name = c->name; t.type = c->type; t.nonVisual = c->nonVisual;
            for (auto& kv : c->events) t.events.push_back({ kv.first, kv.second });
            out.push_back(std::move(t));
        }
        // parent indeksleri
        for (size_t i = 0; i < out.size(); i++) {
            Ctrl* c = g_model.ctrls[i];
            if (c->parent) {
                for (size_t j = 0; j < g_model.ctrls.size(); j++)
                    if (g_model.ctrls[j] == c->parent) { out[i].parentIdx = (int)j; break; }
            }
        }
        return out;
    }
    // parklı form: tasarım JSON'undan oku
    const json::Value& arr = g_docs[formIdx].design.Get(L"Controls");
    if (!arr.IsArr()) return out;
    std::map<std::wstring, int> nameToIdx;
    for (auto& o : arr.arr) {
        TreeObj t;
        t.name = o.Get(L"name").AsStr();
        t.type = o.Get(L"type").AsStr();
        const json::Value& ev = o.Get(L"Events");
        if (ev.IsObj()) for (auto& kv : ev.obj) t.events.push_back({ kv.first, kv.second.AsStr() });
        nameToIdx[t.name] = (int)out.size();
        out.push_back(std::move(t));
    }
    for (size_t i = 0; i < out.size(); i++) {
        std::wstring p = arr.arr[i].Get(L"Parent").AsStr();
        if (!p.empty()) {
            auto it = nameToIdx.find(p);
            if (it != nameToIdx.end() && it->second != (int)i) out[i].parentIdx = it->second;
        }
    }
    return out;
}

std::vector<std::pair<std::wstring, std::wstring>> TreeFormEventsOf(int formIdx) {
    std::vector<std::pair<std::wstring, std::wstring>> out;
    if (formIdx < 0 || formIdx >= (int)g_docs.size()) return out;
    if (formIdx == g_active) {
        for (auto& kv : g_model.formEvents) out.push_back({ kv.first, kv.second });
        return out;
    }
    const json::Value& fe = g_docs[formIdx].design.Get(L"Form").Get(L"Events");
    if (fe.IsObj()) for (auto& kv : fe.obj) out.push_back({ kv.first, kv.second.AsStr() });
    return out;
}

} // namespace forms
