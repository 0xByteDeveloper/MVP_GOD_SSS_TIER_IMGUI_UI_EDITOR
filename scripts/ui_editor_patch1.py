#!/usr/bin/env python3
# ui_editor_patch1.py — DrawToolbox + DrawCodetree + HandleShortcuts yenileme
import re, io

p = "/home/z/my-project/work/god_editor/src/ui_editor.cpp"
src = open(p, encoding="utf-8").read()

BS = "\\"  # literal backslash

# ---- 1) toolbox item bloğu: arm + çift tık ekle -----------------------------
old_tb = (
    "            for (auto& e : g_catalog.catElements[cat]) {\n"
    "                ImGui::PushID(e.c_str());\n"
    "                if (ImGui::Selectable(e.c_str(), canvas::g_armTool == e)) {\n"
)
assert old_tb in src, "toolbox anchor yok"
# eski bloğu tamamen bul: PushID ... PopID arası (satır satır)
start = src.index(old_tb)
end = src.index("                ImGui::PopID();", start) + len("                ImGui::PopID();\n")
new_tb = (
    "            for (auto& e : g_catalog.catElements[cat]) {\n"
    "                ImGui::PushID(e.c_str());\n"
    "                // tık = YERLEŞTİRME MODU (arm — ESC iptal); çift tık = aktif forma direkt ekle\n"
    "                if (ImGui::Selectable(e.c_str(), canvas::IsArmed() && canvas::g_armTool == e,\n"
    "                                      ImGuiSelectableFlags_AllowDoubleClick)) {\n"
    "                    if (ImGui::IsMouseDoubleClicked(0)) {\n"
    "                        canvas::ArmTool(e);\n"
    "                        canvas::AddArmedToForm();\n"
    "                        canvas::CancelArm();\n"
    "                        s_status = \"Eklendi: \" + e + \" (aktif forma)\";\n"
    "                    } else {\n"
    "                        canvas::ArmTool(e);\n"
    "                        s_status = \"Yerle" + BS + "305" + BS + "237tir: \" + e + \" — forma t" + BS + "304" + BS + "261kla (ESC iptal)\";\n"
    "                    }\n"
    "                }\n"
    "                if (ImGui::IsItemHovered()) {\n"
    "                    const ElementDef* d = g_catalog.Find(e);\n"
    "                    if (d) ImGui::SetTooltip(\"%s" + BS + "n%s" + BS + "nT" + BS + "304" + BS + "261kla: yerle" + BS + "305" + BS + "237tir · " + BS + "303" + BS + "247ift t" + BS + "304" + BS + "261kla: forma ekle\",\n"
    "                                             d->cppApi.c_str(), d->kind.c_str());\n"
    "                }\n"
    "                ImGui::PopID();\n"
)
src = src[:start] + new_tb + src[end:]

# ---- 2) Kod Gezgini: SAĞLAM seçim (Selectable satırlar + hiyerarşi) --------
old_ct_anchor = "static void DrawCodetree(ImVec2 min, ImVec2 max) {"
i0 = src.index(old_ct_anchor)
i1 = src.index("static void DrawFormList", i0)  # sonraki fonksiyon
new_ct = r'''// kod gezgini satırı — SAĞLAM seçim (Selectable) + bağlam menüsü + hiyerarşi
static void DrawCtrlRow(Ctrl* c, int formIdx, int depth) {
    ImGui::PushID(c->name.c_str());
    ImGui::PushID(depth);
    std::string line = c->name + "  :  " + c->type;
    bool sel = g_model.IsSelected(c);
    bool container = IsContainerType(c->type) && !c->nonVisual && c->type != "Menu";
    if (container) {
        ImGuiTreeNodeFlags fl = ImGuiTreeNodeFlags_DefaultOpen | (sel ? ImGuiTreeNodeFlags_Selected : 0);
        bool open = ImGui::TreeNodeEx(line.c_str(), fl);
        if (ImGui::IsItemClicked(0)) {
            if (formIdx != forms::ActiveIndex()) forms::SwitchTo(formIdx);
            canvas::Select(c, ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("T\304\261kla: se\303\247 · a\303\247/kapa: \303\247ocuklar");
        if (open) {
            for (Ctrl* ch : g_model.ChildrenOf(c)) DrawCtrlRow(ch, formIdx, depth + 1);
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Selectable(line.c_str(), sel)) {
            if (formIdx != forms::ActiveIndex()) forms::SwitchTo(formIdx);
            canvas::Select(c, ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("T\304\261kla: se\303\247");
    }
    // bağlam menüsü (sağ tık)
    if (ImGui::BeginPopupContextItem("##ctx")) {
        if (ImGui::MenuItem("Se\303\247")) canvas::Select(c, false);
        ImGui::Separator();
        if (ImGui::MenuItem("Kopyala (Ctrl+C)", nullptr, false, g_model.IsSelected(c))) g_model.CopySelection();
        if (ImGui::MenuItem("Kes (Ctrl+X)", nullptr, false, g_model.IsSelected(c))) { g_model.PushUndo(); g_model.CutSelection(); }
        ImGui::Separator();
        if (ImGui::MenuItem("\303\226ne Getir")) g_model.BringToFront(c);
        if (ImGui::MenuItem("Arkaya G\303\266nder")) g_model.SendToBack(c);
        ImGui::Separator();
        if (ImGui::MenuItem("Sil (Del)")) { g_model.PushUndo(); g_model.SelectOnly(c); g_model.DeleteList(g_model.ValidSelection()); }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::PopID();
}

static void DrawCodetree(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::Begin("Kod Gezgini", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    for (size_t i = 0; i < forms::Count(); i++) {
        forms::FormEntry* f = forms::At(i);
        ImGui::PushID((int)i);
        ImGuiTreeNodeFlags fl = ImGuiTreeNodeFlags_DefaultOpen | ((int)i == forms::ActiveIndex() ? ImGuiTreeNodeFlags_Selected : 0);
        bool open = ImGui::TreeNodeEx(f->name.c_str(), fl);
        if (ImGui::IsItemClicked(0)) {
            forms::SwitchTo((int)i);
            g_model.SelectForm();      // forma tıkla = form seçimi
        }
        if (open) {
            // objeler (hiyerarşik: konteyner çocukları ağaçta)
            for (Ctrl* c : f->model.ctrls)
                if (!c->parent) DrawCtrlRow(c, (int)i, 0);
            // form olayları
            if (!f->model.formEvents.empty()) {
                ImGui::Indent(18);
                for (auto& ev : f->model.formEvents)
                    ImGui::BulletText("%s \u2192 %s", (f->name + "_" + ev.first).c_str(), ev.second.c_str());
                ImGui::Unindent(18);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}

'''
src = src[:i0] + new_ct + src[i1:]

open(p, "w", encoding="utf-8").write(src)
print("PATCH1 OK — toolbox + codetree yenilendi")
