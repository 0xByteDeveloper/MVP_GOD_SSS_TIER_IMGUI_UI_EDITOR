// props.cpp — Özellikler + Olaylar paneli (sağ alt)
#include "ui_editor.h"
#include "model.h"
#include "forms.h"
#include "catalog.h"
#include "imgui.h"
#include <cstdio>

namespace ui_editor {

extern bool g_showToolbox, g_showCodetree, g_showForms, g_showProps;
extern std::string g_clipboardJson;

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
    } else {
        char buf[512];
        snprintf(buf, sizeof buf, "%s", val->c_str());
        if (ImGui::InputText("##v", buf, sizeof buf)) { *val = buf; changed = true; }
    }
    ImGui::PopID();
    return changed;
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
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("\303\207ift t\304\261kla: i\305\237leyici g\303\276ster");
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (handler->empty()) *handler = HandlerOf(objName, ev.name);
        ui_editor::StatusUpdate("Olay: " + *handler + " — kod \303\274retiminde i\305\237leyici olu\305\237turulur");
    }
    ImGui::PopID();
}

// ---------------------------------------------------------------------------
//  ana panel
void DrawPropsPanel(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("##props", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    if (ImGui::BeginTabBar("##ptabs")) {
        if (ImGui::BeginTabItem("\303\226zellikler")) {
            Model& m = g_model;
            const ElementDef* def = nullptr;
            if (m.selected) def = g_catalog.Find(m.selected->type);
            if (m.formSelected) {
                // form özellikleri
                ImGui::Text("Form: %s  (%d x %d)", m.formProps.count("Name") ? m.formProps["Name"].c_str() : "Form1", m.formW, m.formH);
                if (ImGui::BeginTable("##fp", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
                    ImGui::TableSetupColumn("Ad", ImGuiTableColumnFlags_WidthFixed, 110);
                    ImGui::TableSetupColumn("De\304\237er", ImGuiTableColumnFlags_WidthStretch);
                    std::string* t = &m.formProps["Text"];
                    if (PropRow("Text", t, nullptr)) g_modelDirty = true;
                    std::string* nm = &m.formProps["Name"];
                    if (PropRow("Name", nm, nullptr)) { forms::RenameActive(*nm); g_modelDirty = true; }
                    std::string cs = m.formProps.count("ClientSize") ? m.formProps["ClientSize"] : std::to_string(m.formW) + ", " + std::to_string(m.formH);
                    if (PropRow("ClientSize", &cs, nullptr)) {
                        int w, h; if (sscanf(cs.c_str(), "%d, %d", &w, &h) == 2 || sscanf(cs.c_str(), "%d,%d", &w, &h) == 2) {
                            m.formW = ImMax(w, 120); m.formH = ImMax(h, 80);
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
                                        if (!fl.empty() && (fl.front() == '|')) fl.erase(0, 1);
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
                    ImGui::EndTable();
                }
            } else if (m.selected && def) {
                ImGui::Text("%s : %s", m.selected->name.c_str(), m.selected->type.c_str());
                if (ImGui::BeginTable("##p", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Ad", ImGuiTableColumnFlags_WidthFixed, 110);
                    ImGui::TableSetupColumn("De\304\237er", ImGuiTableColumnFlags_WidthStretch);
                    // konum/boyut her nesnede
                    {
                        char pv[64];
                        snprintf(pv, sizeof pv, "%d, %d", m.selected->x, m.selected->y);
                        std::string pos = pv;
                        if (PropRow("Konum", &pos, nullptr)) {
                            int x, y; sscanf(pos.c_str(), "%d, %d", &x, &y);
                            m.selected->x = x; m.selected->y = y;
                            g_modelDirty = true;
                        }
                        snprintf(pv, sizeof pv, "%d, %d", m.selected->w, m.selected->h);
                        std::string sz = pv;
                        if (PropRow("Boyut", &sz, nullptr)) {
                            int w, h; sscanf(sz.c_str(), "%d, %d", &w, &h);
                            m.selected->w = w; m.selected->h = h;
                            g_modelDirty = true;
                        }
                    }
                    for (auto& p : def->props) {
                        std::string* val = &m.selected->props[p.name];
                        if (PropRow(p.name.c_str(), val, &p)) g_modelDirty = true;
                    }
                    // Name en sonda (yeniden adlandırma)
                    std::string* nm = &m.selected->props["Name"];
                    if (PropRow("Name", nm, nullptr)) {
                        std::string clean = EscIdent(*nm);
                        if (!clean.empty() && g_model.FindByName(clean) != m.selected) {
                            g_model.Rename(m.selected, clean);
                        } else if (!clean.empty()) {
                            m.selected->name = clean;
                        }
                        g_modelDirty = true;
                    } else {
                        m.selected->props["Name"] = m.selected->name;
                    }
                    ImGui::EndTable();
                }
            } else {
                ImGui::TextUnformatted("Se\303\247im yok");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Olaylar")) {
            Model& m = g_model;
            const ElementDef* def = nullptr;
            if (m.selected) def = g_catalog.Find(m.selected->type);
            if (m.formSelected) {
                ImGui::Text("Form olaylar\304\261 — %s", m.formProps.count("Name") ? m.formProps["Name"].c_str() : "Form1");
                if (ImGui::BeginTable("##fe", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
                    ImGui::TableSetupColumn("Olay", ImGuiTableColumnFlags_WidthFixed, 110);
                    ImGui::TableSetupColumn("\304\260\305\236leyici", ImGuiTableColumnFlags_WidthStretch);
                    // form olayı: Load
                    std::string emptyDef = "";
                    std::string h = m.formEvents.count("Load") ? m.formEvents["Load"] : "";
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Load");
                    ImGui::TableNextColumn();
                    char buf[256];
                    snprintf(buf, sizeof buf, "%s", h.c_str());
                    if (ImGui::InputText("##fh", buf, sizeof buf)) m.formEvents["Load"] = buf;
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        if (m.formEvents["Load"].empty())
                            m.formEvents["Load"] = HandlerOf(m.formProps.count("Name") ? m.formProps["Name"] : "Form1", "Load");
                    }
                    ImGui::EndTable();
                }
            } else if (m.selected && def) {
                ImGui::Text("%s — olaylar", m.selected->name.c_str());
                if (ImGui::BeginTable("##e", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Olay", ImGuiTableColumnFlags_WidthFixed, 110);
                    ImGui::TableSetupColumn("\304\260\305\237leyici", ImGuiTableColumnFlags_WidthStretch);
                    for (auto& ev : def->events) {
                        std::string* h = &m.selected->events[ev.name];
                        EventRow(ev, h, m.selected->name.c_str());
                    }
                    ImGui::EndTable();
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
