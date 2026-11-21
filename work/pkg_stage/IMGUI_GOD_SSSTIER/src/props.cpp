// props.cpp — Özellikler + Olaylar paneli (sağ alt) — kategorik property grid.
// Bölümler: Görünüm / Yerleşim / Veri / Davranış / RENK / STİL.
// Renk satırları: ColorEdit paleti + hex + "kalıtıma dön" düğmesi.
// Tüm satırlar PushID + "##" etiketli → ImGui ID çakışması imkansız.
#include "ui_editor.h"
#include "model.h"
#include "forms.h"
#include "catalog.h"
#include "objstyle.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace ui_editor {

extern bool g_showToolbox, g_showCodetree, g_showForms, g_showProps;

// ---------------------------------------------------------------------------
//  özellik satırı editörleri
static bool PropRow(const char* name, std::string* val, const PropDef* def) {
    bool changed = false;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(name);
    if (def && !def->hint.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", def->hint.c_str());
    ImGui::TableNextColumn();
    ImGui::PushID(name);
    if (!def || def->type == "string") {
        char buf[512];
        snprintf(buf, sizeof buf, "%s", val->c_str());
        if (ImGui::InputText("##v", buf, sizeof buf)) { *val = buf; changed = true; }
    } else if (def->type == "bool") {
        bool b = (*val == "True" || *val == "true" || *val == "1");
        if (ImGui::Checkbox("##v", &b)) { *val = b ? "True" : "False"; changed = true; }
    } else if (def->type == "int") {
        int v = atoi(val->c_str());
        if (ImGui::InputInt("##v", &v, 0)) { *val = std::to_string(v); changed = true; }
    } else if (def->type == "float") {
        float v = (float)atof(val->c_str());
        if (ImGui::InputFloat("##v", &v, 0, 0, "%.3g")) { *val = std::to_string(v); changed = true; }
    } else if (def->type == "items") {
        char buf[2048];
        snprintf(buf, sizeof buf, "%s", val->c_str());
        if (ImGui::InputTextMultiline("##v", buf, sizeof buf, ImVec2(-1, 44))) { *val = buf; changed = true; }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("\303\226\304\237eler | ile ayr\304\261l\304\261r");
    } else if (def->type == "color") {
        // renk: "" = kalıtım. Palet + hex + sıfırla
        float f[4];
        bool has = objstyle::ParseHexColor(*val, f);
        if (!has) { f[0] = f[1] = f[2] = 0.45f; f[3] = 1.0f; }
        ImGui::PushID("col");
        if (ImGui::ColorEdit3("##cv", f, ImGuiColorEditFlags_NoInputs)) {
            *val = objstyle::HexOf(f);
            changed = true;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("hex");
        char hb[16];
        snprintf(hb, sizeof hb, "%s", val->c_str());
        ImGui::SetNextItemWidth(70);
        if (ImGui::InputText("##hx", hb, sizeof hb, ImGuiInputTextFlags_AutoSelectAll)) {
            float tf[4];
            if (objstyle::ParseHexColor(std::string(hb), tf)) { *val = objstyle::HexOf(tf); changed = true; }
            else if (strlen(hb) == 0) { *val = ""; changed = true; }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("rst");
        ImGui::BeginDisabled(val->empty());
        if (ImGui::SmallButton("x")) { *val = ""; changed = true; }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Temadan kal\304\261t\304\261m");
        ImGui::PopID();
        if (val->empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("Bo\305\237 = tema rengi (kal\304\261t\304\261m)");
    } else {
        char buf[512];
        snprintf(buf, sizeof buf, "%s", val->c_str());
        if (ImGui::InputText("##v", buf, sizeof buf)) { *val = buf; changed = true; }
    }
    ImGui::PopID();
    return changed;
}

// kategori sırası (panel görüntü düzeni)
static const char* CatOrder[] = { "G\303\266r\303\274n\303\274m", "Yerle\305\237im", "Veri", "Davran\304\261\305\237", "Renk", "Stil" };
static const int NCat = 6;
static const char* CatName(const std::string& c) {
    if (c == "Görünüm") return CatOrder[0];
    if (c == "Yerleşim") return CatOrder[1];
    if (c == "Veri") return CatOrder[2];
    if (c == "Davranış") return CatOrder[3];
    if (c == "Renk") return CatOrder[4];
    if (c == "Stil") return CatOrder[5];
    return CatOrder[0];
}

// ---------------------------------------------------------------------------
//  olay satırı
static void EventRow(const EventDef& ev, std::string* handler, const char* objName) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(ev.name.c_str());
    if (ImGui::IsItemHovered() && !ev.desc.empty()) ImGui::SetTooltip("%s", ev.desc.c_str());
    ImGui::TableNextColumn();
    char buf[256];
    snprintf(buf, sizeof buf, "%s", handler->c_str());
    ImGui::PushID(ev.name.c_str());
    if (ImGui::InputText("##h", buf, sizeof buf)) *handler = buf;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("\303\207ift t\304\261kla: i\305\237leyici \303\274ret");
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (handler->empty()) *handler = HandlerOf(objName, ev.name);
        ui_editor::StatusUpdate("Olay: " + *handler + " — kod \303\274retiminde i\305\237leyici olu\305\237turulur");
    }
    ImGui::PopID();
}

// iki sütunlu tablo başlat — BeginTable başarısızsa (pencere skip) güvenli çık
static bool BeginPropTable(const char* id) {
    ImGui::PushID(id);
    if (!ImGui::BeginTable("##t", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                      ImGuiTableFlags_SizingFixedFit)) {
        ImGui::PopID();
        return false;
    }
    ImGui::TableSetupColumn("Ad", ImGuiTableColumnFlags_WidthFixed, 108);
    ImGui::TableSetupColumn("De\304\237er", ImGuiTableColumnFlags_WidthStretch);
    return true;
}
static void EndPropTable() {
    ImGui::EndTable();
    ImGui::PopID();
}

// ---------------------------------------------------------------------------
//  ana panel
void DrawPropsPanel(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("##props", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    if (ImGui::BeginTabBar("##ptabs")) {
        if (ImGui::BeginTabItem("\303\226zellikler")) {
            Model& m = g_model;
            const ElementDef* def = nullptr;
            if (m.Primary()) def = g_catalog.Find(m.Primary()->type);
            if (m.formSelected) {
                // ---------------- form özellikleri ----------------
                ImGui::Text("Form: %s  (%d x %d)", m.formProps.count("Name") ? m.formProps["Name"].c_str() : "Form1", m.formW, m.formH);
                bool _tf = BeginPropTable("form");
                if (_tf) {
                std::string* t = &m.formProps["Text"];
                if (PropRow("Text", t, nullptr)) g_modelDirty = true;
                std::string* nm = &m.formProps["Name"];
                if (PropRow("Name", nm, nullptr)) { forms::RenameActive(*nm); g_modelDirty = true; }
                std::string cs = m.formProps.count("ClientSize") ? m.formProps["ClientSize"] : std::to_string(m.formW) + ", " + std::to_string(m.formH);
                if (PropRow("ClientSize", &cs, nullptr)) {
                    int w, h;
                    if (sscanf(cs.c_str(), "%d, %d", &w, &h) == 2 || sscanf(cs.c_str(), "%d,%d", &w, &h) == 2) {
                        m.formW = (w > 120 ? w : 120); m.formH = (h > 80 ? h : 80);
                        m.formProps["ClientSize"] = std::to_string(m.formW) + ", " + std::to_string(m.formH);
                    }
                    g_modelDirty = true;
                }
                // Flags: çoklu seçim açılır listesi
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Flags");
                ImGui::TableNextColumn();
                std::string fl = m.formProps.count("Flags") ? m.formProps["Flags"] : "";
                std::string cur = fl.empty() ? "(yok)" : fl;
                if (ImGui::BeginCombo("##fl", cur.c_str())) {
                    const char* opts[] = { "NoTitleBar", "NoResize", "NoCollapse", "NoMove", "AlwaysAutoResize", "NoScrollbar", "MenuBar" };
                    for (auto& o : opts) {
                        bool sel = fl.find(o) != std::string::npos;
                        if (ImGui::MenuItem(o, nullptr, sel)) {
                            if (sel) {
                                size_t p = fl.find(o);
                                if (p != std::string::npos) {
                                    fl.erase(p, strlen(o));
                                    if (!fl.empty() && fl.front() == '|') fl.erase(0, 1);
                                    if (!fl.empty() && fl.back() == '|') fl.pop_back();
                                }
                            } else {
                                if (!fl.empty()) fl += "|";
                                fl += o;
                            }
                            m.formProps["Flags"] = fl;
                            g_modelDirty = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                // form rengi/stili
                {
                    std::string bg = m.formProps.count("WindowBgColor") ? m.formProps["WindowBgColor"] : std::string("");
                    PropDef bgDef; bgDef.name = "WindowBgColor"; bgDef.type = "color"; bgDef.category = "Renk";
                    bgDef.hint = "Form zemini (bo\305\237 = tema)";
                    if (PropRow("WindowBgColor", &bg, &bgDef)) { m.formProps["WindowBgColor"] = bg; g_modelDirty = true; }
                    std::string rnd = m.formProps.count("WindowRounding") ? m.formProps["WindowRounding"] : std::string("-1");
                    PropDef rndDef; rndDef.name = "WindowRounding"; rndDef.type = "float"; rndDef.hint = "K\303\266\305\237e yuvarlama (-1 = tema)";
                    if (PropRow("WindowRounding", &rnd, &rndDef)) { m.formProps["WindowRounding"] = rnd; g_modelDirty = true; }
                    std::string bs = m.formProps.count("WindowBorderSize") ? m.formProps["WindowBorderSize"] : std::string("-1");
                    PropDef bsDef; bsDef.name = "WindowBorderSize"; bsDef.type = "float"; bsDef.hint = "Kenarl\304\261k px (-1 = tema)";
                    if (PropRow("WindowBorderSize", &bs, &bsDef)) { m.formProps["WindowBorderSize"] = bs; g_modelDirty = true; }
                }
                } // _tf
                if (_tf) EndPropTable();
            } else if (m.Primary() && def) {
                // ---------------- nesne özellikleri (kategorik) ----------------
                size_t nSel = m.selList.size();
                if (nSel > 1)
                    ImGui::Text("%s : %s   (+%d daha se\303\247ili)", m.Primary()->name.c_str(), m.Primary()->type.c_str(), (int)nSel - 1);
                else
                    ImGui::Text("%s : %s", m.Primary()->name.c_str(), m.Primary()->type.c_str());

                bool _t0 = BeginPropTable("obj0");
                if (_t0) {
                    char pv[64];
                    snprintf(pv, sizeof pv, "%d, %d", m.Primary()->x, m.Primary()->y);
                    std::string pos = pv;
                    if (PropRow("Konum", &pos, nullptr)) {
                        int x, y;
                        if (sscanf(pos.c_str(), "%d, %d", &x, &y) == 2) { m.Primary()->x = x; m.Primary()->y = y; }
                        else { m.Primary()->x = atoi(pos.c_str()); }
                        g_modelDirty = true;
                    }
                    // BOYUT SATIRI — TİP YETENEĞİNE GÖRE (kullanıcı hatası: "görselde
                    // küçülmüyordu ama özelliklerde Boyut değişiyordu" — hayalet
                    // boyut kökten giderildi):FULL=düzenlenebilir w,h; W=yalnız
                    // genişlik (yükseklik ImGui sabidi, "(otomatik)"); AUTO=içerikten.
                    int cap = SizeCapOf(m.Primary());
                    if (cap == SIZECAP_FULL) {
                        snprintf(pv, sizeof pv, "%d, %d", m.Primary()->w, m.Primary()->h);
                        std::string sz = pv;
                        if (PropRow("Boyut", &sz, nullptr)) {
                            int w, h;
                            if (sscanf(sz.c_str(), "%d, %d", &w, &h) == 2) {
                                m.Primary()->w = (w > 12 ? w : 12); m.Primary()->h = (h > 8 ? h : 8);
                                // özellik paneli ↔ Width/Height/SizeArg senkronu (tek gerçek)
                                auto sw = m.Primary()->props.find("Width");   if (sw != m.Primary()->props.end()) sw->second = std::to_string(m.Primary()->w);
                                auto sh = m.Primary()->props.find("Height"); if (sh != m.Primary()->props.end()) sh->second = std::to_string(m.Primary()->h);
                                auto szw = m.Primary()->props.find("SizeArgW"); if (szw != m.Primary()->props.end()) szw->second = std::to_string(m.Primary()->w);
                                auto szh = m.Primary()->props.find("SizeArgH"); if (szh != m.Primary()->props.end()) szh->second = std::to_string(m.Primary()->h);
                            }
                            g_modelDirty = true;
                        }
                    } else if (cap == SIZECAP_W) {
                        // yalnız GENİŞLİK ayarlanabilir — SetNextItemWidth uygulanır
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Geni\305\237lik");
                        ImGui::TableNextColumn();
                        int wv = m.Primary()->w;
                        ImGui::PushID("wonly");
                        if (ImGui::InputInt("##v", &wv, 0)) {
                            if (wv < 24) wv = 24;
                            m.Primary()->w = wv;
                            auto sw = m.Primary()->props.find("Width");
                            if (sw != m.Primary()->props.end()) sw->second = std::to_string(wv);
                            g_modelDirty = true;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bu denetimin geni\305\237li\304\237i \303\247izimde ve \303\303\274retilen kodda aynen uygulan\304\261r");
                        ImGui::PopID();
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Y\303\274kseklik");
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(otomatik)");
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Bu denetimin y\303\274ksekli\304\237i ImGui taraf\304\261ndan sabitlenir \342\200\224 geni\305\237lik ayarlanabilir");
                    } else {
                        // içerik boyutlu — deği\305\237tirilemez, gerçek boyut gösterilir
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted("Boyut");
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("(otomatik \342\200\224 i\303\247erikten \303\266l\303\247\303\274l\303\274r)");
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Bu denetimin boyutu metninden/i\303\247eri\304\237inden gelir \342\200\224 ImGui evreninde de\304\237i\305\237tirilemez");
                    }
                }
                if (_t0) EndPropTable();

                // kategori bölümleri (CollapsingHeader — hepsi açık)
                for (int ci = 0; ci < NCat; ci++) {
                    // bu kategorideki özellikler (tanım sırasıyla)
                    std::vector<const PropDef*> catProps;
                    for (auto& p : def->props)
                        if (CatName(p.category) == CatOrder[ci]) catProps.push_back(&p);
                    if (catProps.empty()) continue;
                    ImGui::PushID(ci);
                    if (ImGui::CollapsingHeader(CatOrder[ci], ImGuiTreeNodeFlags_DefaultOpen) && BeginPropTable("cat")) {
                        for (auto* p : catProps) {
                            std::string* val = &m.Primary()->props[p->name];
                            if (PropRow(p->name.c_str(), val, p)) {
                                // ÇOKLU DÜZENLEME: seçimdeki diğer AYNI TİP nesnelere de
                                // uygula (toplu görünüm/davranış değiştirme — M2God tarzı)
                                if (m.selList.size() > 1) {
                                    for (Ctrl* oc : m.ValidSelection())
                                        if (oc != m.Primary() && oc->type == m.Primary()->type)
                                            oc->props[p->name] = *val;
                                }
                                // WYSIWYG SENKRONU: "Width/Height" (ve SizeArgW/H) prop'u
                                // değişince TASARIM BOYUTU da güncellenir — canvas ve üretilen
                                // kod daima aynı boyutu kullanır (prop ↔ boyut tek kaynak).
                                if (p->name == "Width" || p->name == "SizeArgW") {
                                    int v = atoi(val->c_str());
                                    if (v > 0) m.Primary()->w = v;
                                }
                                if (p->name == "Height" || p->name == "SizeArgH") {
                                    int v = atoi(val->c_str());
                                    if (v > 0) m.Primary()->h = v;
                                }
                                // ListBox: satır sayısı ↔ tasarım yüksekliği (iki yön
                                // tutarlı — görsel ile sayı daima aynı)
                                if (p->name == "HeightItems") {
                                    int items = atoi(val->c_str());
                                    if (items >= 1) {
                                        float hh = ImGui::GetTextLineHeightWithSpacing() * (items + 0.25f)
                                                   + ImGui::GetStyle().FramePadding.y * 2.0f;
                                        m.Primary()->h = (int)(hh + 0.5f);
                                    }
                                }
                                g_modelDirty = true;
                            }
                        }
                        EndPropTable();
                    }
                    ImGui::PopID();
                }

                // Name (yeniden adlandırma) — en altta
                bool _tn = BeginPropTable("objname");
                if (_tn) {
                    std::string* nmv = &m.Primary()->props["Name"];
                    if (PropRow("Name", nmv, nullptr)) {
                        std::string clean = EscIdent(*nmv);
                        if (!clean.empty() && g_model.FindByName(clean) != m.Primary()) {
                            g_model.Rename(m.Primary(), clean);
                        } else if (!clean.empty()) {
                            m.Primary()->name = clean;
                        }
                        g_modelDirty = true;
                    } else {
                        m.Primary()->props["Name"] = m.Primary()->name;
                    }
                }
                if (_tn) EndPropTable();
            } else {
                ImGui::TextUnformatted("Se\303\247im yok");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Olaylar")) {
            Model& m = g_model;
            const ElementDef* def = nullptr;
            if (m.Primary()) def = g_catalog.Find(m.Primary()->type);
            if (m.formSelected) {
                ImGui::Text("Form olaylar\304\261 — %s", m.formProps.count("Name") ? m.formProps["Name"].c_str() : "Form1");
                bool _te = BeginPropTable("fev");
                if (_te) {
                    std::string h = m.formEvents.count("Load") ? m.formEvents["Load"] : "";
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Load");
                    ImGui::TableNextColumn();
                    char buf[256];
                    snprintf(buf, sizeof buf, "%s", h.c_str());
                    ImGui::PushID("fload");
                    if (ImGui::InputText("##fh", buf, sizeof buf)) m.formEvents["Load"] = buf;
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        if (m.formEvents["Load"].empty())
                            m.formEvents["Load"] = HandlerOf(m.formProps.count("Name") ? m.formProps["Name"] : "Form1", "Load");
                    }
                    ImGui::PopID();
                    EndPropTable();
                }
            } else if (m.Primary() && def) {
                ImGui::Text("%s — olaylar", m.Primary()->name.c_str());
                if (BeginPropTable("ev")) {
                    for (auto& ev : def->events) {
                        std::string* h = &m.Primary()->events[ev.name];
                        EventRow(ev, h, m.Primary()->name.c_str());
                    }
                    EndPropTable();
                }
            } else {
                ImGui::TextUnformatted("Se\303\247im yok");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

} // namespace ui_editor
