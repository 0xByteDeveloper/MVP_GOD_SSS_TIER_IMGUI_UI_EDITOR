// forms.cpp — çok formlu proje katmanı
#include "forms.h"
#include <sstream>

namespace forms {

static std::vector<FormEntry> g_forms;
static int g_active = 0;
static std::string g_projectDir;

void Init() {
    g_forms.clear();
    FormEntry f;
    f.name = "Form1";
    f.parked = false;
    f.model.formW = 430; f.model.formH = 460;
    f.model.formProps["Name"] = "Form1";
    f.model.formProps["Text"] = "Form1";
    f.model.formProps["ClientSize"] = "430, 460";
    g_forms.push_back(std::move(f));
    g_active = 0;
    g_projectDir = "";
}
size_t Count() { return g_forms.size(); }
int ActiveIndex() { return g_active; }
FormEntry* Active() { return g_active >= 0 && g_active < (int)g_forms.size() ? &g_forms[g_active] : nullptr; }
FormEntry* At(size_t i) { return i < g_forms.size() ? &g_forms[i] : nullptr; }
std::vector<FormEntry>& All() { return g_forms; }

std::string NewForm(const std::string& baseName) {
    // Form2, Form3... çakışmayan ad
    int n = (int)g_forms.size() + 1;
    std::string name = baseName.empty() ? "Form" + std::to_string(n) : baseName;
    while (true) {
        bool clash = false;
        for (auto& f : g_forms) if (f.name == name) { clash = true; break; }
        if (!clash) break;
        name = "Form" + std::to_string(++n);
    }
    FormEntry f;
    f.name = name;
    f.parked = false;
    f.model.formW = 430; f.model.formH = 460;
    f.model.formProps["Name"] = name;
    f.model.formProps["Text"] = name;
    f.model.formProps["ClientSize"] = "430, 460";
    // diğerlerini park et
    for (auto& ff : g_forms) ff.parked = true;
    g_forms.push_back(std::move(f));
    g_active = (int)g_forms.size() - 1;
    g_modelDirty = true;
    return name;
}
void RenameActive(const std::string& newName) {
    FormEntry* f = Active();
    if (!f || newName.empty()) return;
    for (auto& ff : g_forms) if (ff.name == newName && &ff != f) return;  // çakışma
    f->name = newName;
    f->model.formProps["Name"] = newName;
    if (f->model.formProps.count("Text") && f->model.formProps["Text"] == "")
        f->model.formProps["Text"] = newName;
    g_modelDirty = true;
}
bool DeleteAt(int index) {
    if (g_forms.size() <= 1) return false;    // son form silinemez
    if (index < 0 || index >= (int)g_forms.size()) return false;
    g_forms.erase(g_forms.begin() + index);
    if (g_active >= (int)g_forms.size()) g_active = (int)g_forms.size() - 1;
    if (g_active >= 0) g_forms[g_active].parked = false;
    g_modelDirty = true;
    return true;
}
void SwitchTo(int index) {
    if (index < 0 || index >= (int)g_forms.size()) return;
    for (size_t i = 0; i < g_forms.size(); i++) g_forms[i].parked = ((int)i != index);
    g_active = index;
    g_forms[index].parked = false;
}

std::string ProjectDir() { return g_projectDir; }
void SetProjectDir(const std::string& d) { g_projectDir = d; }

// --- tek dosya kayıt -------------------------------------------------------
bool SaveProjectAs(const std::string& path) {
    json::Value root = json::Value::MakeObj();
    SetS(root, "format", "imgod-project");
    root.Set("version", (double)4);
    root.Set("FormCount", (double)g_forms.size());
    root.Set("Active", (double)g_active);
    json::Value arr = json::Value::MakeArr();
    for (auto& f : g_forms) {
        json::Value fv = f.model.ToJson();
        fv.Set("FormName", f.name);
        arr.Push(fv);
    }
    root.Set("Forms", arr);
    bool ok = WriteFileTextA(path, json::Stringify(root, true) + "\n");
    if (ok) {
        SetProjectDir(DirOfA(path));
        g_modelDirty = false;
    }
    return ok;
}

bool LoadProject(const std::string& path) {
    std::string txt;
    if (!ReadFileTextA(path, txt)) return false;
    bool ok = false;
    json::Value root = json::Parse(txt, &ok);
    if (!ok || !root.IsObj()) return false;
    // klasör projesi mi? (Forms.info)
    if (BaseNameA(path) == "Forms.info") {
        SetProjectDir(DirOfA(path));
        return LoadFolderProject(path);
    }
    const json::Value& arr = root.Get(L"Forms");
    if (arr.t != json::Value::ARR) {
        // belki tek-form .imgod (model.ToJson) — doğrudan yükle
        g_forms.clear();
        FormEntry f; f.name = "Form1"; f.parked = false;
        if (f.model.FromJson(root)) { g_forms.push_back(std::move(f)); g_active = 0; return true; }
        return false;
    }
    g_forms.clear();
    for (auto& fv : arr.arr) {
        FormEntry f;
        f.name = WideStrToUtf8(fv.Get(L"FormName").AsStr(L"Form1"));
        if (f.name.empty()) f.name = "Form" + std::to_string(g_forms.size() + 1);
        f.model.FromJson(fv);
        f.parked = true;
        g_forms.push_back(std::move(f));
    }
    if (g_forms.empty()) { Init(); return false; }
    g_active = (int)root.Get(L"Active").AsNum(0);
    if (g_active < 0 || g_active >= (int)g_forms.size()) g_active = 0;
    g_forms[g_active].parked = false;
    SetProjectDir(DirOfA(path));
    g_modelDirty = false;
    return true;
}

// --- klasör projesi (Forms.info + FormN.info) -------------------------------
bool SaveFolderProject(const std::string& dir) {
    if (!MkDirsA(dir)) return false;
    std::string infoPath = JoinPathA(dir, "Forms.info");
    json::Value root = json::Value::MakeObj();
    SetS(root, "format", "imgod-folder");
    root.Set("FormCount", (double)g_forms.size());
    root.Set("Active", (double)g_active);
    json::Value names = json::Value::MakeArr();
    for (auto& f : g_forms) names.Push(json::Value::MakeStr(Utf8ToWideStr(f.name)));
    root.Set("Names", names);
    // proje genelinde aktif renderer/backend tercihini de sakla
    if (!WriteFileTextA(infoPath, json::Stringify(root, true) + "\n")) return false;
    for (auto& f : g_forms) {
        std::string fp = JoinPathA(dir, f.name + ".info");
        if (!f.model.Save(fp)) return false;
    }
    SetProjectDir(dir);
    g_modelDirty = false;
    return true;
}
bool LoadFolderProject(const std::string& infoPath) {
    std::string txt;
    if (!ReadFileTextA(infoPath, txt)) return false;
    bool ok = false;
    json::Value root = json::Parse(txt, &ok);
    if (!ok || !root.IsObj()) return false;
    std::string dir = DirOfA(infoPath);
    const json::Value& names = root.Get(L"Names");
    g_forms.clear();
    if (names.t == json::Value::ARR) {
        for (auto& nv : names.arr) {
            FormEntry f;
            f.name = WideStrToUtf8(nv.AsStr());
            if (f.name.empty()) continue;
            std::string fp = JoinPathA(dir, f.name + ".info");
            f.model.Load(fp);
            f.parked = true;
            g_forms.push_back(std::move(f));
        }
    }
    if (g_forms.empty()) { Init(); return false; }
    g_active = (int)root.Get(L"Active").AsNum(0);
    if (g_active < 0 || g_active >= (int)g_forms.size()) g_active = 0;
    g_forms[g_active].parked = false;
    SetProjectDir(dir);
    g_modelDirty = false;
    return true;
}

// --- kod gezgini ------------------------------------------------------------
std::vector<Ctrl*> ObjectsOf(int formIndex) {
    std::vector<Ctrl*> out;
    if (formIndex < 0 || formIndex >= (int)g_forms.size()) return out;
    for (Ctrl* c : g_forms[formIndex].model.ctrls) out.push_back(c);
    return out;
}
std::vector<std::pair<std::string, std::string>> FormEventsOf(int formIndex) {
    std::vector<std::pair<std::string, std::string>> out;
    if (formIndex < 0 || formIndex >= (int)g_forms.size()) return out;
    for (auto& kv : g_forms[formIndex].model.formEvents) out.push_back(kv);
    return out;
}

} // namespace forms
