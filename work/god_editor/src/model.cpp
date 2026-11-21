// model.cpp — tasarım modelinin uygulanışı (undo snapshot tabanlı, ImGui props)
#include "model.h"
#include <sstream>

Model g_model;

std::vector<Ctrl*> Model::ChildrenOf(Ctrl* p) const {
    std::vector<Ctrl*> out;
    for (Ctrl* c : ctrls) if (c->parent == p) out.push_back(c);
    // ctrls z sırası: listede son = üstte
    return out;
}
bool Model::IsAncestorOf(Ctrl* a, Ctrl* c) const {
    for (Ctrl* p = c->parent; p; p = p->parent) if (p == a) return true;
    return false;
}
void Model::Reparent(Ctrl* c, Ctrl* newParent, int px, int py) {
    if (newParent == c || (newParent && IsAncestorOf(c, newParent))) return;  // döngü koruması
    c->parent = newParent;
    c->x = px; c->y = py;
    g_modelDirty = true;
}

// --- undo ------------------------------------------------------------------
static json::Value CtrlToJson(const Ctrl* c) {
    json::Value v = json::Value::MakeObj();
    v.Set("Name", c->name);
    v.Set("Type", c->type);
    v.Set("X", (double)c->x);
    v.Set("Y", (double)c->y);
    v.Set("W", (double)c->w);
    v.Set("H", (double)c->h);
    if (c->nonVisual) SetB(v, "NonVisual", true);
    if (c->parent) v.Set("Parent", c->parent->name);
    json::Value ps = json::Value::MakeObj();
    for (auto& kv : c->props) ps.Set(kv.first, kv.second);
    v.Set("Props", ps);
    json::Value es = json::Value::MakeObj();
    for (auto& kv : c->events) es.Set(kv.first, kv.second);
    v.Set("Events", es);
    return v;
}
static void CtrlFromJson(Ctrl* c, const json::Value& v, Model* m) {
    c->name = WideStrToUtf8(v.Get(L"Name").AsStr());
    c->type = WideStrToUtf8(v.Get(L"Type").AsStr());
    c->x = (int)v.Get(L"X").AsNum(20);
    c->y = (int)v.Get(L"Y").AsNum(20);
    c->w = (int)v.Get(L"W").AsNum(0);
    c->h = (int)v.Get(L"H").AsNum(0);
    c->nonVisual = v.Get(L"NonVisual").AsBool(false);
    std::string parentName = WideStrToUtf8(v.Get(L"Parent").AsStr());
    if (!parentName.empty()) {
        Ctrl* p = m->FindByName(parentName);
        if (p) c->parent = p;   // ikinci geçişte çözülür (aşağıda)
    }
    const json::Value& ps = v.Get(L"Props");
    if (ps.t == json::Value::OBJ) for (auto& kv : ps.obj)
        c->props[WideStrToUtf8(kv.first)] = WideStrToUtf8(kv.second.AsStr());
    const json::Value& es = v.Get(L"Events");
    if (es.t == json::Value::OBJ) for (auto& kv : es.obj)
        c->events[WideStrToUtf8(kv.first)] = WideStrToUtf8(kv.second.AsStr());
    // parentName sakla — geç 2'de çözüm için
    if (!parentName.empty()) c->props["__parentRef"] = parentName;
}
static void FixParentsPass2(Model* m) {
    for (Ctrl* c : m->ctrls) {
        auto it = c->props.find("__parentRef");
        if (it != c->props.end()) {
            std::string pn = it->second;
            c->props.erase(it);
            Ctrl* p = m->FindByName(pn);
            if (p) c->parent = p;
        }
    }
}

std::string Model::Snapshot() const {
    json::Value v = ToJson();
    return json::Stringify(v, false);
}
void Model::PushUndo() {
    undo.push_back(Snapshot());
    if (undo.size() > 100) undo.pop_front();
    redo.clear();
    g_modelDirty = true;
}
bool Model::Undo() {
    if (undo.empty()) return false;
    redo.push_back(Snapshot());
    std::string js = undo.back();
    undo.pop_back();
    Restore(js);
    return true;
}
bool Model::Redo() {
    if (redo.empty()) return false;
    undo.push_back(Snapshot());
    std::string js = redo.back();
    redo.pop_back();
    Restore(js);
    return true;
}
void Model::Restore(const std::string& js) {
    bool ok = false;
    json::Value v = json::Parse(js, &ok);
    if (ok) FromJson(v);
    g_modelDirty = true;
}
std::deque<std::string> Model::DetachUndo() { auto u = undo; undo.clear(); return u; }
std::deque<std::string> Model::DetachRedo() { auto r = redo; redo.clear(); return r; }
void Model::AttachUndo(std::deque<std::string> u, std::deque<std::string> r) { undo.swap(u); redo.swap(r); }
void Model::ClearUndo() { undo.clear(); redo.clear(); }

// --- isim / ekleme ---------------------------------------------------------
std::string Model::UniqueName(const std::string& type) {
    std::string low = type;
    for (auto& c : low) c = (char)tolower((unsigned char)c);
    if (!low.empty()) low[0] = (char)toupper((unsigned char)low[0]);
    if (low == "InputTextMultiline") low = "InputTextMultiline";
    int n = nameCounters[low] + 1;
    while (FindByName(low + std::to_string(n)) != nullptr) n++;
    nameCounters[low] = n;
    return low + std::to_string(n);
}
Ctrl* Model::Add(const std::string& type, int x, int y, Ctrl* parent) {
    const ElementDef* def = g_catalog.Find(type);
    if (!def) return nullptr;
    Ctrl* c = new Ctrl();
    c->name = UniqueName(type);
    c->type = type;
    c->x = x; c->y = y;
    c->w = def->defW; c->h = def->defH;
    c->parent = parent;
    c->nonVisual = (def->kind == "component");
    // varsayılan özellikleri katalogtan doldur
    for (auto& p : def->props) c->props[p.name] = p.def;
    ctrls.push_back(c);
    selected = c;
    formSelected = false;
    g_modelDirty = true;
    return c;
}
void Model::Delete(Ctrl* c) {
    if (!c) return;
    // alt kontrolleri ve kendini kaldır
    std::vector<Ctrl*> stack{ c };
    std::vector<Ctrl*> kill;
    while (!stack.empty()) {
        Ctrl* cur = stack.back(); stack.pop_back();
        kill.push_back(cur);
        for (Ctrl* cc : ctrls) if (cc->parent == cur) stack.push_back(cc);
    }
    for (Ctrl* k : kill) {
        ctrls.erase(std::remove(ctrls.begin(), ctrls.end(), k), ctrls.end());
        delete k;
    }
    if (selected == c) { selected = nullptr; formSelected = true; }
    g_modelDirty = true;
}
Ctrl* Model::Clone(Ctrl* src, int dx, int dy) {
    if (!src) return nullptr;
    Ctrl* c = new Ctrl(*src);
    c->name = UniqueName(src->type);
    c->x += dx; c->y += dy;
    // handler adları klon için çeşitlendir
    for (auto& kv : c->events) kv.second = HandlerOf(c->name, kv.first);
    // alt kontrolleri de klonla (aynı sıra ile)
    std::vector<Ctrl*> order;
    // derinlik sırası: ebeveyn önce — basit yaklaşım: kaynak listede sırayla
    for (Ctrl* cc : ctrls) if (cc->parent == src) order.push_back(cc);
    ctrls.push_back(c);
    std::map<Ctrl*, Ctrl*> m2;
    m2[src] = c;
    // yinelemeli klon (çocuk sırası korunur)
    std::function<void(Ctrl*)> cloneKids = [&](Ctrl* from) {
        for (Ctrl* cc : ctrls) {
            if (cc->parent == from) {
                // henüz klonlanmadıysa
                bool cloned = false;
                for (auto& kv : m2) if (kv.second->name == cc->name) { cloned = true; break; }
                if (!cloned) {
                    Ctrl* k = new Ctrl(*cc);
                    k->name = UniqueName(cc->type);
                    k->x = cc->x; k->y = cc->y;
                    for (auto& kv : k->events) kv.second = HandlerOf(k->name, kv.first);
                    k->parent = m2[cc->parent];
                    ctrls.push_back(k);
                    m2[cc] = k;
                    cloneKids(cc);
                }
            }
        }
    };
    cloneKids(src);
    selected = c;
    g_modelDirty = true;
    return c;
}
Ctrl* Model::FindByName(const std::string& name) {
    for (Ctrl* c : ctrls) if (c->name == name) return c;
    return nullptr;
}
void Model::Rename(Ctrl* c, const std::string& newName) {
    if (!c || newName.empty() || FindByName(newName)) return;
    std::string old = c->name;
    c->name = newName;
    // olay handler adlarını yeni isme taşı
    for (auto& kv : c->events) kv.second = HandlerOf(newName, kv.first);
    // çocukların Parent referansları otomatik (pointer) — ek işlem yok
    g_modelDirty = true;
}
void Model::BringToFront(Ctrl* c) {
    if (!c) return;
    ctrls.erase(std::remove(ctrls.begin(), ctrls.end(), c), ctrls.end());
    ctrls.push_back(c);
    g_modelDirty = true;
}
void Model::SendToBack(Ctrl* c) {
    if (!c) return;
    ctrls.erase(std::remove(ctrls.begin(), ctrls.end(), c), ctrls.end());
    ctrls.insert(ctrls.begin(), c);
    g_modelDirty = true;
}
void Model::ClearAll() {
    for (Ctrl* c : ctrls) delete c;
    ctrls.clear();
    selected = nullptr;
    formSelected = true;
}
std::string Model::DefaultEventOf(const Ctrl* c) const {
    if (!c) return "";
    const ElementDef* def = g_catalog.Find(c->type);
    return def ? def->defaultEvent : "";
}

// --- JSON ------------------------------------------------------------------
json::Value Model::ToJson() const {
    json::Value v = json::Value::MakeObj();
    SetS(v, "format", "imgod-project");
    v.Set("version", (double)3);
    json::Value fp = json::Value::MakeObj();
    for (auto& kv : formProps) {
        json::Value s; s.t = json::Value::STR; s.str = Utf8ToWideStr(kv.second);
        fp.obj.push_back({ Utf8ToWideStr(kv.first), s });
    }
    v.Set("FormProps", fp);
    json::Value fe = json::Value::MakeObj();
    for (auto& kv : formEvents) {
        json::Value s; s.t = json::Value::STR; s.str = Utf8ToWideStr(kv.second);
        fe.obj.push_back({ Utf8ToWideStr(kv.first), s });
    }
    v.Set("FormEvents", fe);
    v.Set("FormW", (double)formW);
    v.Set("FormH", (double)formH);
    json::Value arr = json::Value::MakeArr();
    for (Ctrl* c : ctrls) arr.Push(CtrlToJson(c));
    v.Set("Ctrls", arr);
    return v;
}
bool Model::FromJson(const json::Value& v) {
    ClearAll();
    if (!v.IsObj()) return false;
    const json::Value& fp = v.Get(L"FormProps");
    if (fp.t == json::Value::OBJ) for (auto& kv : fp.obj)
        formProps[WideStrToUtf8(kv.first)] = WideStrToUtf8(kv.second.AsStr());
    const json::Value& fe = v.Get(L"FormEvents");
    if (fe.t == json::Value::OBJ) for (auto& kv : fe.obj)
        formEvents[WideStrToUtf8(kv.first)] = WideStrToUtf8(kv.second.AsStr());
    formW = (int)v.Get(L"FormW").AsNum(430);
    formH = (int)v.Get(L"FormH").AsNum(460);
    const json::Value& arr = v.Get(L"Ctrls");
    if (arr.t == json::Value::ARR) {
        for (auto& cv : arr.arr) {
            Ctrl* c = new Ctrl();
            CtrlFromJson(c, cv, this);
            ctrls.push_back(c);
        }
    }
    FixParentsPass2(this);
    // form adı formProps["Name"]
    if (formProps.find("Name") == formProps.end()) {
        std::string t = formProps.count("Text") ? formProps["Text"] : "Form1";
        formProps["Name"] = t.empty() ? "Form1" : t;
    }
    g_modelDirty = true;
    return true;
}
bool Model::Save(const std::string& path) {
    return WriteFileTextA(path, json::Stringify(ToJson(), true) + "\n");
}
bool Model::Load(const std::string& path) {
    std::string txt;
    if (!ReadFileTextA(path, txt)) return false;
    bool ok = false;
    json::Value v = json::Parse(txt, &ok);
    if (!ok) return false;
    return FromJson(v);
}
