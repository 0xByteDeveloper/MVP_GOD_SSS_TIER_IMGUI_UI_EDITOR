#!/usr/bin/env python3
# ui_editor_patch2.py — HandleShortcuts (çoklu seçim klavyesi) + form listesi + misc
p = "/home/z/my-project/work/god_editor/src/ui_editor.cpp"
src = open(p, encoding="utf-8").read()

BS = "\\"

# ---- 1) HandleShortcuts: tam yeniden yazım -------------------------------
i0 = src.index("static void HandleShortcuts(ImGuiIO& io) {")
i1 = src.index("// ---------------------------------------------------------------------------\n//  selftest", i0)
new_hs = r'''static void HandleShortcuts(ImGuiIO& io) {
    // metin girişi odaktaysa kısayolları yeme (panellerdeki InputText)
    if (io.WantTextInput) return;
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) { g_model.Undo(); s_status = "Geri al\304\261nd\304\261"; }
        else if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) { g_model.Redo(); s_status = "Yinelendi"; }
        else if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            // Ctrl+C — seçili nesneleri kopyala (alt kontrollerle birlikte)
            if (g_model.CopySelection()) {
                size_t n = g_model.selList.size();
                s_status = "Kopyaland\304\261: " + std::to_string(n) + " nesne (Ctrl+V ile yap\304\261\305\237t\304\261r)";
            } else s_status = "Kopyalanacak nesne se\303\247ili de\304\237il";
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
            // Ctrl+X — kes
            if (!g_model.selList.empty()) {
                g_model.PushUndo();
                size_t n = g_model.selList.size();
                if (g_model.CutSelection()) s_status = "Kesildi: " + std::to_string(n) + " nesne";
            }
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            // Ctrl+V — yapıştır (yeni adlar TİP sayacıyla: denemebutton → button3)
            if (!g_model.clipboardJson.empty()) {
                g_model.PushUndo();
                int n = g_model.PasteClipboard(GRID_SIZE, GRID_SIZE);
                s_status = "Yap\304\261\305\237t\304\261r\304\261ld\304\261: " + std::to_string(n) + " nesne (yeni adlarla)";
            }
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
            // Ctrl+A — tümünü seç
            g_model.SelectAll();
            s_status = "T\303\274m\303\274 se\303\247ildi: " + std::to_string(g_model.selList.size()) + " nesne";
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_S, false)) DoSave(false);
        else if (ImGui::IsKeyPressed(ImGuiKey_O, false)) DoOpen();
        else if (ImGui::IsKeyPressed(ImGuiKey_N, false)) DoNewProject();
    } else {
        if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) s_genDialog = true;
        else if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) g_gridVisible = !g_gridVisible;
        else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !g_model.selList.empty()) {
            // Del — seçili NESNELERİ sil (form seçiliyken formu silmez)
            g_model.PushUndo();
            size_t n = g_model.selList.size();
            g_model.DeleteList(g_model.ValidSelection());
            s_status = "Silindi: " + std::to_string(n) + " nesne";
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (canvas::IsArmed()) { canvas::CancelArm(); s_status = "Yerle\305\237tirme iptal edildi"; }
            else { g_model.SelectForm(); s_status = "Se\303\247im temizlendi"; }
        }
        else if (!g_model.selList.empty()) {
            // ok tuşları: İnce ayar (Shift = ızgara adımı) — seçili GRUBU taşı
            int step = io.KeyShift ? GRID_SIZE : 1;
            int dx = 0, dy = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) dx = -step;
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) dx = step;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) dy = -step;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) dy = step;
            if (dx || dy) {
                for (Ctrl* c : g_model.ValidSelection()) { c->x += dx; c->y += dy; }
                g_modelDirty = true;
            }
        }
    }
}

'''
src = src[:i0] + new_hs + src[i1:]

# ---- 2) eski küresel pano değişkenlerini kaldır (model kendi panosunu tutar) ----
src = src.replace(
    'bool g_showToolbox = true, g_showCodetree = true, g_showForms = true, g_showProps = true;\nstd::string g_clipboardJson;\nCtrl* g_clipOwner = nullptr;\n',
    'bool g_showToolbox = true, g_showCodetree = true, g_showForms = true, g_showProps = true;\n')

open(p, "w", encoding="utf-8").write(src)
print("PATCH2 OK — shortcuts + globals")
