// model.cpp — tasarım modelinin uygulanışı (undo snapshot tabanlı, ImGui props)
#include "model.h"
#include <sstream>

Model g_model;

// --- çoklu seçim -------------------------------------------------------------
bool Model::IsSelected(const Ctrl* c) const {
    for (Ctrl* s : selList) if (s == c) return true;
    return false;
}
std::vector<Ctrl*> Model::ValidSelection() const {
    std::vector<Ctrl*> out;
    for (Ctrl* s : selList) {
        for (Ctrl* c : ctrls) if (c == s) { out.push_back(s); break; }
    }
    return out;
}
void Model::SelectOnly(Ctrl* c) {
    selList.clear();
    if (c) selList.push_back(c);
    selected = c;
    formSelected = (c == nullptr);
}
void Model::SetSelection(const std::vector<Ctrl*>& list) {
    selList.clear();
    for (Ctrl* c : list) {
        bool exists = false;
        for (Ctrl* x : ctrls) if (x == c) { exists = true; break; }
        if (exists && !IsSelected(c)) selList.push_back(c);
    }
    selected = selList.empty() ? nullptr : selList.back();
    formSelected = selList.empty();
}
void Model::ToggleSel(Ctrl* c) {
    if (!c) { SelectForm(); return; }
    if (IsSelected(c)) {
        selList.erase(std::remove(selList.begin(), selList.end(), c), selList.end());
        if (selList.empty()) SelectForm();
        else selected = selList.back();
    } else {
        selList.push_back(c);
        selected = c;
        formSelected = false;
    }
}
void Model::AddSel(Ctrl* c) {
    if (!c) return;
    if (!IsSelected(c)) { selList.push_back(c); selected = c; formSelected = false; }
}
void Model::SelectForm() {
    selList.clear();
    selected = nullptr;
    formSelected = true;
}
void Model::SelectAll() {
    selList = ctrls;
    selected = selList.empty() ? nullptr : selList.back();
    formSelected = selList.empty();
}
void Model::SyncSelection() {
    bool changed = false;
    std::vector<Ctrl*> keep;
    for (Ctrl* s : selList) {
        bool exists = false;
        for (Ctrl* c : ctrls) if (c == s) { exists = true; break; }
        if (exists) keep.push_back(s); else changed = true;
    }
    selList.swap(keep);
    selected = selList.empty() ? nullptr : selList.back();
    if (selList.empty()) formSelected = true;
    (void)changed;
}

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

// --- boyut yeteneği (tek gerçek kaynak) -------------------------------------
// HATA DÜZELTİLDİ (kullanıcı: "combo/listbox gibi şeyler görsel olarak büyüyüp
// küçülmüyo, ama özelliklerdeki Boyut değişiyo"): birçok widget tipi çizimde
// c->w/c->h'yi HİÇ uygulamıyordu — yalnız özellik panelindeki sayı değişiyordu.
// Artık her tipin GERÇEK ImGui boyut yeteneği tek yerde tanımlı ve canvas,
// özellik paneli VE kod üretimi bu tek kaynağı kullanıyor:
//   SIZECAP_FULL (2) — hem genişlik hem yükseklik (ImVec2 argüman)
//   SIZECAP_W    (1) — yalnız genişlik (SetNextItemWidth; yükseklik ImGui sabiti)
//   SIZECAP_AUTO (0) — içerikten ölçülür (özellikte "(otomatik)" gösterilir)
int SizeCapOf(const Ctrl* c) {
    if (!c) return SIZECAP_AUTO;
    const std::string& t = c->type;
    if (t == "Button")          return c->GetB("Small") ? SIZECAP_AUTO : SIZECAP_FULL; // SmallButton içerik ölçer
    if (t == "ProgressBar" || t == "PlotLines" || t == "PlotHistogram" ||
        t == "InputTextMultiline" || t == "ListBox" ||
        t == "Panel" || t == "ChildWindow" || t == "GroupBox")
                                 return SIZECAP_FULL;
    if (t == "Combo" || t == "SliderFloat" || t == "SliderInt" ||
        t == "DragFloat" || t == "DragInt" || t == "InputText" ||
        t == "InputInt" || t == "InputFloat" || t == "Table")
                                 return SIZECAP_W;      // SetNextItemWidth / outer_size.x
    if (t == "ColorEdit4")       return c->GetB("AlphaPreview") ? SIZECAP_W : SIZECAP_AUTO; // NoInputs kare içerik ölçer
    return SIZECAP_AUTO;   // Checkbox, RadioButton, Text*, Separator*, CollapsingHeader, TreeNode, TabBar...
}

// --- isim / ekleme ---------------------------------------------------------
std::string Model::UniqueName(const std::string& type) {
    // WinForms/M2God adlandırma: tip adının İLK HARFİ KÜÇÜK (button1, radioButton1,
    // sliderFloat1, inputTextMultiline1...) — özgün tasarımcı davranışıyla birebir.
    std::string low = type;
    if (!low.empty()) low[0] = (char)tolower((unsigned char)low[0]);
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
    SelectOnly(c);
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
    SyncSelection();
    g_modelDirty = true;
}
void Model::DeleteList(const std::vector<Ctrl*>& list) {
    // alt kontrollerle birlikte — tek tek Delete() yerine tek geçiş
    std::set<Ctrl*> kill;
    std::vector<Ctrl*> stack(list.begin(), list.end());
    while (!stack.empty()) {
        Ctrl* cur = stack.back(); stack.pop_back();
        if (kill.insert(cur).second)
            for (Ctrl* cc : ctrls) if (cc->parent == cur) stack.push_back(cc);
    }
    for (Ctrl* k : kill) {
        ctrls.erase(std::remove(ctrls.begin(), ctrls.end(), k), ctrls.end());
        delete k;
    }
    SyncSelection();
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
    SelectOnly(c);
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
    SelectForm();
    clipboardJson.clear();
    pasteCount = 0;
}
std::string Model::DefaultEventOf(const Ctrl* c) const {
    if (!c) return "";
    const ElementDef* def = g_catalog.Find(c->type);
    return def ? def->defaultEvent : "";
}

// --- pano (Ctrl+C/X/V — M2God davranışı: yeni adlar TİP sayacıyla) -------------
bool Model::CopySelection() {
    std::vector<Ctrl*> sel = ValidSelection();
    if (sel.empty()) return false;
    // üst-seviye seçimler + bunların alt ağaçları (konteyner seçimi tüm içeriği kopyalar)
    std::set<Ctrl*> full;
    std::vector<Ctrl*> stack(sel.begin(), sel.end());
    while (!stack.empty()) {
        Ctrl* cur = stack.back(); stack.pop_back();
        if (full.insert(cur).second)
            for (Ctrl* cc : ctrls) if (cc->parent == cur) stack.push_back(cc);
    }
    json::Value arr = json::Value::MakeArr();
    for (Ctrl* c : ctrls)
        if (full.count(c)) arr.Push(CtrlToJson(c));
    clipboardJson = json::Stringify(arr, false);
    pasteCount = 0;
    return true;
}
bool Model::CutSelection() {
    std::vector<Ctrl*> sel = ValidSelection();
    if (sel.empty()) return false;
    if (!CopySelection()) return false;
    DeleteList(sel);
    return true;
}
Ctrl* Model::PasteCtrlFromJson(const json::Value& v, const std::map<std::string, std::string>& nameMap,
                               Ctrl* parentOverride, int dx, int dy) {
    Ctrl* c = new Ctrl();
    CtrlFromJson(c, v, this);
    // YENİ AD: tip sayacıyla (özgün M2God: denemebutton → button3)
    std::string newName = UniqueName(c->type);
    c->name = newName;
    c->x += dx; c->y += dy;
    // olay işleyici adlarını yeni ada bağla
    for (auto& kv : c->events) kv.second = HandlerOf(newName, kv.first);
    c->props.erase("__parentRef");
    c->props.erase("Name");
    c->props["Name"] = newName;
    if (parentOverride) c->parent = parentOverride;
    ctrls.push_back(c);
    return c;
}
int Model::PasteClipboard(int dx, int dy) {
    if (clipboardJson.empty()) return 0;
    bool ok = false;
    json::Value arr = json::Parse(clipboardJson, &ok);
    if (!ok || arr.t != json::Value::ARR) return 0;
    pasteCount++;
    // kademeli ofset: her yapıştırma bir ızgara kaydır
    int ox = dx + (pasteCount - 1) * GRID_SIZE;
    int oy = dy + (pasteCount - 1) * GRID_SIZE;
    // 1. geçiş: adları yeniden üret (parent referansları için ad haritası kur)
    std::map<std::string, std::string> nameMap;   // eski ad → yeni ad
    std::vector<Ctrl*> fresh;
    for (auto& cv : arr.arr) {
        std::string oldName = WideStrToUtf8(cv.Get(L"Name").AsStr());
        Ctrl* nc = PasteCtrlFromJson(cv, nameMap, nullptr, ox, oy);
        if (nc) { nameMap[oldName] = nc->name; fresh.push_back(nc); }
    }
    // 2. geçiş: parent zincirini geri kur (kaynak parent da kopyalandıysa yeni kopyaya bağla)
    std::set<Ctrl*> freshSet(fresh.begin(), fresh.end());
    for (size_t i = 0; i < arr.arr.size() && i < fresh.size(); i++) {
        json::Value cv = arr.arr[i];
        std::string pn = WideStrToUtf8(cv.Get(L"Parent").AsStr());
        if (!pn.empty()) {
            auto it = nameMap.find(pn);
            if (it != nameMap.end()) {
                Ctrl* np = FindByName(it->second);
                if (np && freshSet.count(np)) fresh[i]->parent = np;
            } else {
                fresh[i]->parent = nullptr;   // parent kopyalanmadı → form seviyesi
            }
        }
    }
    // seçim: yapıştırılan üst seviyeler (alt kontroller ebeveynleriyle taşınır)
    std::vector<Ctrl*> top;
    for (Ctrl* nc : fresh)
        if (!nc->parent || !freshSet.count(nc->parent)) top.push_back(nc);
    SetSelection(top);
    g_modelDirty = true;
    return (int)fresh.size();
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
