// ============================================================================
//  ui_editor.cpp — IMGUI_GOD_SSSTIER editör kabuğu.
//  * Başlangıç ekranı: VS seç & bağlan | önceki tasarımı aç | VS'siz devam + motor bilgisi
//  * Ana kabuk: menü çubuğu + araç çubuğu + 4 panel (Kod Gezgini, Araç Kutusu,
//    Formlar, Özellikler/Olaylar) + canvas + durum çubuğu + sürüklenebilir ayraçlar
//  * Üretim diyaloğu: D3D sürümü seçimi (dx9/dx10/dx11/dx12/opengl3) + tema + hedef
// ============================================================================
#include "ui_editor.h"
#include "model.h"
#include "forms.h"
#include "catalog.h"
#include "theme.h"
#include "vsint.h"
#include "codegen.h"
#include "canvas.h"
#include <windows.h>
#include <commdlg.h>
#include "imgui_internal.h"
#include <cstdio>
#include <sstream>

namespace ui_editor {

bool g_showToolbox = true, g_showCodetree = true, g_showForms = true, g_showProps = true;
std::string g_clipboardJson;
Ctrl* g_clipOwner = nullptr;

static std::string s_status = "HAZIR";
static std::string s_selftestPath;
static std::string s_pendingOpen;
static bool s_genDialog = false;             // üretim diyaloğu açık
static bool s_aboutOpen = false;
static int s_editorTheme = theme::TImGuiDark;

// ---- diyaloglar (comdlg — Wine altında da çalışır) ----
std::string OpenDlg(const char* title, const char* filter) {
    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrFilter = filter;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return file;
    return "";
}
std::string SaveDlg(const char* title, const char* filter, const char* defExt) {
    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn = { sizeof(ofn) };
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrFilter = filter;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) return file;
    return "";
}
void StatusUpdate(const std::string& s) { s_status = s; }
void SetSelftest(const std::string& p) { s_selftestPath = p; }
void SetPendingOpen(const std::string& p) { s_pendingOpen = p; }

bool ToolbarButton(const char* label, const char* desc, bool enabled) {
    ImGui::BeginDisabled(!enabled);
    bool c = ImGui::SmallButton(label);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", desc); StatusUpdate(desc); }
    ImGui::EndDisabled();
    return c;
}

// ---------------------------------------------------------------------------
//  başlangıç ekranı — ÖNCE VS seçimi, sonra sıfır form (özgün akış)
struct WelcomeState {
    std::vector<vsint::VSInstance> insts;
    int sel = 0;
    bool scanned = false;
    bool connectFailed = false;
    std::string connectMsg;
    int editorBackendView = (int)EditorBackend::DX11;
};
static WelcomeState s_w;

static void DrawWelcome(EditorState* es, ImGuiIO& io) {
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 330, io.DisplaySize.y * 0.5f - 260));
    ImGui::SetNextWindowSize(ImVec2(660, 520));
    ImGui::Begin("IMGUI GOD SSS TIER — Ba\305\237lang\304\241\303\247", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(90, 220, 120, 255));
    ImGui::TextUnformatted("IMGUI GOD SSS TIER — UI Edit\303\266r ve Kod \303\234reticisi");
    ImGui::PopStyleColor();
    ImGui::TextUnformatted("VS2019 C++ ile derlenince edit\303\266rdeki formu AYNEN g\303\266steren ImGui projeleri \303\274retir.");
    ImGui::Separator();

    ImGui::TextUnformatted("1) \303\207al\304\261\305\237an Visual Studio se\303\247in ve ba\304\237lan\304\261n:");
    if (!s_w.scanned) { s_w.insts = vsint::Enumerate(); s_w.scanned = true; }
    ImGui::SetNextItemWidth(-1);
    if (!s_w.insts.empty()) {
        // satır listesi (native combo)
        const char* names[64];
        for (size_t i = 0; i < s_w.insts.size() && i < 64; i++) {
            static char bufs[64][160];
            snprintf(bufs[i], sizeof bufs[i], "PID %lu — %s", s_w.insts[i].pid,
                     s_w.insts[i].title.c_str());
            names[i] = bufs[i];
        }
        ImGui::ListBox("##vs", &s_w.sel, names, (int)(s_w.insts.size() < 64 ? s_w.insts.size() : 64), 6);
    } else {
        ImGui::TextDisabled("(Çalışan Visual Studio bulunamadı — devenv.exe)");
        ImGui::TextWrapped("Wine / Linux altında VS yoktur: aşağıdaki seçeneklerle devam edebilirsiniz.");
    }
    if (ImGui::Button("Yenile")) { s_w.insts = vsint::Enumerate(); s_w.sel = 0; }
    ImGui::SameLine();
    ImGui::BeginDisabled(s_w.insts.empty() || s_w.sel >= (int)s_w.insts.size());
    if (ImGui::Button("VS'ye Ba\304\237lan")) {
        auto& inst = s_w.insts[s_w.sel];
        if (!inst.solutionPath.empty() && vsint::Connect(inst.solutionPath, "")) {
            s_status = "VS ba\304\237land\304\261: " + inst.solutionPath;
            g_modelDirty = true;
            es->welcome = 0;   // edit\303\266re ge\303\247
        } else {
            // \303\247\303\266z\303\274m yolu diskte bulunamad\304\261 — klas\303\266r taramas\304\261 iste
            s_w.connectMsg = "\303\207\303\266z\303\274m .sln yolu bulunamad\304\261 — dosya se\303\247im diyalo\304\237u a\303\247\304\261l\304\261yor.";
            std::string sln = OpenDlg("Visual Studio \303\207\303\266z\303\274m\303\274 (.sln) se\303\247", "\303\207\303\266z\303\274m\303\274 dosyas\304\261\0*.sln\0T\303\274m dosyalar\0*.*\0");
            if (!sln.empty() && vsint::Connect(sln, "")) {
                s_status = "VS ba\304\237land\304\261: " + sln;
                vsint::OpenSolution(sln);   // Windows'ta VS'yi a\303\247ar
                es->welcome = 0;
            } else s_w.connectMsg = "Ba\304\237lan\304\261lamad\304\261 — VS'siz devam edin.";
        }
    }
    ImGui::EndDisabled();
    ImGui::TextWrapped("Ba\304\237lan\304\261nca \303\247\303\266z\303\274mdeki proje klas\303\266r\303\274ne \303\274retim yap\304\261l\304\261r ve VS penceresi \303\266ne getirilir.");
    if (!s_w.connectMsg.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 120, 90, 255));
        ImGui::TextWrapped("%s", s_w.connectMsg.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::Separator();

    ImGui::TextUnformatted("2) \303\226nceki tasar\304\261m\304\261 a\303\247:");
    if (ImGui::Button("A\303\247 (.imgod / Forms.info)")) {
        std::string p = OpenDlg("Proje a\303\247", "IMGUI GOD projesi\0*.imgod\0Forms.info\0Forms.info\0T\303\274m dosyalar\0*.*\0");
        if (!p.empty() && forms::LoadProject(p)) {
            s_status = "A\303\247\304\261ld\304\261: " + p;
            es->welcome = 0;
        } else if (!p.empty()) s_status = "A\303\247\304\261lamad\304\261: " + p;
    }
    ImGui::Separator();

    ImGui::TextUnformatted("3) VEYA VS'siz devam:");
    if (ImGui::Button("VS'siz bo\305\237 formla devam et", ImVec2(240, 34))) {
        es->welcome = 0;
        s_status = "VS'siz mod — bo\305\237 Form1";
    }
    ImGui::Separator();
    ImGui::Text("Edit\303\266r grafik motoru: %s   (tam s\303\274r\303\274m seti \303\274retimde: DX9/DX10/DX11/DX12/OpenGL3)",
                BackendNameA((EditorBackend)es->editorBackend));
    ImGui::End();
    // diyalog mesajları metinleri düzgün yaz: yukarıda escape kullanıldı — karakter sorunu yok (utf-8 kaynak)
}

// ---------------------------------------------------------------------------
//  yardımcı: dosya kaydet/aç akışları
static void DoNewProject() {
    forms::Init();
    g_modelDirty = false;
    s_status = "Yeni proje — Form1";
}
static void DoOpen() {
    std::string p = OpenDlg("Proje a\303\247", "IMGUI GOD projesi\0*.imgod\0Forms.info\0Forms.info\0T\303\274m dosyalar\0*.*\0");
    if (!p.empty() && forms::LoadProject(p)) s_status = "A\303\247\304\261ld\304\261: " + p;
    else if (!p.empty()) s_status = "A\303\247\304\261lamad\304\261";
}
static void DoSave(bool saveAs) {
    if (saveAs) {
        std::string p = SaveDlg("Proje kaydet", "IMGUI GOD projesi\0*.imgod\0", "imgod");
        if (p.empty()) return;
        if (forms::SaveProjectAs(p)) s_status = "Kaydedildi: " + p;
        else s_status = "Kaydedilemedi";
        return;
    }
    if (!forms::ProjectDir().empty()) {
        if (forms::SaveFolderProject(forms::ProjectDir())) s_status = "Kaydedildi: " + forms::ProjectDir();
        else s_status = "Kaydedilemedi";
    } else DoSave(true);
}
static void DoSaveFolder() {
    std::string info = SaveDlg("Klas\303\266r projesi kaydet (Forms.info)", "Forms.info\0Forms.info\0", "info");
    if (info.empty()) return;
    if (forms::SaveFolderProject(DirOfA(info))) s_status = "Klas\303\266r projesi: " + DirOfA(info);
    else s_status = "Kaydedilemedi";
}

// ---------------------------------------------------------------------------
//  kod üretim diyaloğu — D3D SÜRÜMÜ SEÇİMİ (kullanıcı talebi)
static void DrawGenDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 320, ImGui::GetIO().DisplaySize.y * 0.5f - 200));
    ImGui::SetNextWindowSize(ImVec2(640, 400));
    ImGui::Begin("Kod \303\234ret — Ayarlar", &s_genDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted("D3D / Grafik S\303\274r\303\274m\303\274 (derlenecek uygulaman\304\261n motoru):");
    const char* bes[] = { "DirectX 11 (varsay\304\261lan)", "DirectX 9", "DirectX 10", "DirectX 12", "OpenGL 3" };
    ImGui::SetNextItemWidth(300);
    ImGui::Combo("##backend", &codegen::g_settings.backend, bes, IM_ARRAYSIZE(bes));
    ImGui::TextUnformatted("Tema (derlenecek uygulama ba\305\237lang\304\261\303\247 temas\304\261):");
    {
        const char* ths[6];
        for (auto& t : theme::List()) ths[t.id] = t.name;
        ImGui::SetNextItemWidth(300);
        ImGui::Combo("##theme", &codegen::g_settings.themeId, ths, 6);
    }
    ImGui::Checkbox("VS2019 \303\247\303\266z\303\274m\303\274 \303\274ret (.sln + .vcxproj v142)", &codegen::g_settings.genVS2019);
    ImGui::Checkbox("MinGW derleme beti\304\237i (build_mingw.sh)", &codegen::g_settings.genMinGW);
    ImGui::Checkbox("x64 hedefi (x86 da dahil edilir)", &codegen::g_settings.x64);
    char pn[128];
    snprintf(pn, sizeof pn, "%s", codegen::g_settings.projectName.c_str());
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("Proje ad\304\261", pn, sizeof pn)) codegen::g_settings.projectName = pn;
    ImGui::Separator();
    ImGui::TextWrapped("\304\260mge klas\303\266r\303\274: ImGui SDK kopyalan\304\261r, her form <Ad>.cpp/.h \303\274retilir, olay i\305\237leyicileri bo\305\237 g\303\266vdeyle olu\305\237turulur.");
    if (ImGui::Button("\303\234ret (klas\303\266r se\303\247)", ImVec2(180, 36))) {
        std::string p = SaveDlg("\303\234retim klas\303\266r\303\274", "Klas\303\266r se\303\247im\0\0", "");
        if (!p.empty()) {
            p = DirOfA(p);   // bir dosya seçilirse klasörüne
            if (p.empty()) p = ".";
            if (codegen::EmitProject(p)) {
                s_status = "\303\234retildi: " + p + " (" + std::string(BackendNameA((EditorBackend)codegen::g_settings.backend)) + ")";
                // VS bağlıysa çözümü aç
                if (vsint::IsConnected()) vsint::OpenSolution(vsint::ConnectedSln());
            } else s_status = "\303\234retilemedi: " + p;
        }
        s_genDialog = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("\304\260ptal")) s_genDialog = false;
    ImGui::End();
}

// ---------------------------------------------------------------------------
//  paneller
static void DrawToolbox(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::Begin("Ara\303\247 Kutusu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    static char search[128] = "";
    ImGui::TextUnformatted("Ara (kategori/nesne):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##tbs", search, sizeof search);
    ImGui::Separator();
    for (auto& cat : g_catalog.categories) {
        // arama filtresi
        bool show = search[0] == 0;
        for (auto& e : g_catalog.catElements[cat]) {
            std::wstring low = LowerW(Utf8ToWideStr(e));
            std::wstring sw = LowerW(Utf8ToWideStr(std::string(search)));
            if (low.find(sw) != std::wstring::npos || cat.find(search) != std::string::npos) show = true;
        }
        if (!show) continue;
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(cat.c_str())) {
            for (auto& e : g_catalog.catElements[cat]) {
                ImGui::PushID(e.c_str());
                if (ImGui::Selectable(e.c_str(), canvas::g_armTool == e)) {
                    canvas::g_armTool = e;   // yerleştirme moduna gir
                    s_status = "Yerle\305\237tir: " + e + " — canvas'a t\304\261kla";
                }
                if (ImGui::IsItemHovered()) {
                    const ElementDef* d = g_catalog.Find(e);
                    if (d) ImGui::SetTooltip("%s\n%s\n\303\227ift t\304\261kla yerle\305\237tir, canvas'ta konum se\303\247",
                                             d->cppApi.c_str(), d->kind.c_str());
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
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
        if (ImGui::IsItemClicked()) forms::SwitchTo((int)i);
        if (open) {
            // objeler
            for (Ctrl* c : f->model.ctrls) {
                ImGui::BulletText("%s : %s", c->name.c_str(), c->type.c_str());
                if (ImGui::IsItemClicked()) {
                    if ((int)i != forms::ActiveIndex()) forms::SwitchTo((int)i);
                    canvas::Select(c);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tek t\304\261kla: se\303\247");
                // olayları iç içe
                if (!c->events.empty()) {
                    ImGui::Indent(18);
                    for (auto& ev : c->events) ImGui::BulletText("%s \u2192 %s", ev.first.c_str(), ev.second.c_str());
                    ImGui::Unindent(18);
                }
            }
            // form olayları
            for (auto& ev : f->model.formEvents)
                ImGui::BulletText("%s \u2192 %s", (f->name + "_" + ev.first).c_str(), ev.second.c_str());
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}

static void DrawFormList(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::Begin("Formlar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    for (size_t i = 0; i < forms::Count(); i++) {
        forms::FormEntry* f = forms::At(i);
        ImGui::PushID((int)i);
        std::string line = f->name + "   (" + std::to_string(f->model.ctrls.size()) + " nesne)";
        if (ImGui::Selectable(line.c_str(), (int)i == forms::ActiveIndex())) forms::SwitchTo((int)i);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            static char nb[128];
            snprintf(nb, sizeof nb, "%s", f->name.c_str());
            ImGui::OpenPopup("Yeniden Adland\304\261r");
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::Button("+ Yeni Form")) { forms::NewForm(); s_status = "Yeni form eklendi"; }
    ImGui::SameLine();
    ImGui::BeginDisabled(forms::Count() <= 1);
    if (ImGui::Button("Sil")) { forms::DeleteAt(forms::ActiveIndex()); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Yen. Ad")) ImGui::OpenPopup("Yeniden Adland\304\261r");
    if (ImGui::BeginPopup("Yeniden Adland\304\261r")) {
        static char nb[128];
        snprintf(nb, sizeof nb, "%s", forms::Active()->name.c_str());
        if (ImGui::InputText("Yeni ad", nb, sizeof nb)) {}
        if (ImGui::Button("Tamam")) { forms::RenameActive(nb); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("\304\260ptal")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::End();
}

void DrawPropsPanel(ImVec2 min, ImVec2 max);   // props.cpp'de

// ---------------------------------------------------------------------------
//  klavye kısayolları
static void HandleShortcuts(ImGuiIO& io) {
    if (io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) { g_model.Undo(); s_status = "Geri al\304\261nd\304\261"; }
        else if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) { g_model.Redo(); s_status = "Yinelendi"; }
        else if (ImGui::IsKeyPressed(ImGuiKey_C, false) && g_model.selected) {
            // Ctrl+C — tek nesne JSON kopyala
            std::ostringstream o;
            o << "{";
            Ctrl* c = g_model.selected;
            o << "\"Name\":\"" << c->name << "\",\"Type\":\"" << c->type << "\",\"X\":" << c->x << ",\"Y\":" << c->y;
            o << ",\"W\":" << c->w << ",\"H\":" << c->h << ",\"Props\":{";
            bool first = true;
            for (auto& kv : c->props) {
                if (!first) o << ",";
                o << "\"" << kv.first << "\":\"" << kv.second << "\"";
                first = false;
            }
            o << "}}";
            g_clipboardJson = o.str();
            s_status = "Kopyaland\304\261: " + c->name;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_X, false) && g_model.selected) {
            // basitçe kopyala + sil
            ImGuiIO io2 = io; (void)io2;
            g_model.PushUndo();
            g_model.Delete(g_model.selected);
            s_status = "Kesildi";
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_S, false)) DoSave(false);
        else if (ImGui::IsKeyPressed(ImGuiKey_O, false)) DoOpen();
    } else {
        if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) s_genDialog = true;
        else if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) g_gridVisible = !g_gridVisible;
        else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && g_model.selected) {
            g_model.PushUndo();
            g_model.Delete(g_model.selected);
            s_status = "Silindi";
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) canvas::g_armTool.clear();
        else if (g_model.selected) {
            // ok tuşları: ince ayar (Shift = 8px)
            int step = io.KeyShift ? GRID_SIZE : 1;
            int dx = 0, dy = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) dx = -step;
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) dx = step;
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) dy = -step;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) dy = step;
            if (dx || dy) { g_model.selected->x += dx; g_model.selected->y += dy; g_modelDirty = true; }
        }
    }
}

// ---------------------------------------------------------------------------
//  selftest otomasyonu — Wine altında bağımsız test için
//  Betik satırları:  add <tip> <x> <y> ; select <isim> ; set <isim> <prop> <değer>
//  form <x> <y> ; newform ; save <yol> ; gen <yol> ; quit ; wait <kare>
static int s_selftestFrame = 0;
static std::vector<std::string> s_selftestLines;
static bool s_selftestActive = false;
static int s_selftestWaitUntil = -1;
static void SelftestStep() {
    if (!s_selftestActive) return;
    if (s_selftestWaitUntil >= 0) {
        if (s_selftestFrame < s_selftestWaitUntil) return;
        s_selftestWaitUntil = -1;
    }
    while (!s_selftestLines.empty()) {
        std::string line = TrimA(s_selftestLines.front());
        s_selftestLines.erase(s_selftestLines.begin());
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string cmd; iss >> cmd;
        if (cmd == "add") {
            std::string type; int x, y;
            iss >> type >> x >> y;
            Ctrl* c = g_model.Add(type, x, y, nullptr);
            s_status = c ? ("Eklendi: " + c->name) : "Ekleme hatas\304\261";
        } else if (cmd == "select") {
            std::string n; iss >> n;
            canvas::Select(g_model.FindByName(n));
        } else if (cmd == "set") {
            std::string n, p; iss >> n >> p;
            std::string rest; std::getline(iss, rest);
            rest = TrimA(rest);
            Ctrl* c = g_model.FindByName(n);
            if (c) {
                if (p == "Name") g_model.Rename(c, rest);
                else c->props[p] = rest;
            }
        } else if (cmd == "form") { iss >> g_model.formW >> g_model.formH; g_model.formProps["ClientSize"] = std::to_string(g_model.formW) + ", " + std::to_string(g_model.formH); }
        else if (cmd == "newform") { forms::NewForm(); }
        else if (cmd == "save") { std::string p; std::getline(iss, p); p = TrimA(p); forms::SaveProjectAs(p); }
        else if (cmd == "savefolder") { std::string p; std::getline(iss, p); p = TrimA(p); forms::SaveFolderProject(p); }
        else if (cmd == "gen") { std::string p; std::getline(iss, p); p = TrimA(p); codegen::EmitProject(p); }
        else if (cmd == "theme") { int t; iss >> t; theme::Apply(t); }
        else if (cmd == "genbackend") { int b; iss >> b; codegen::g_settings.backend = b; }
        else if (cmd == "wait") { int k; iss >> k; s_selftestWaitUntil = s_selftestFrame + k; return; }
        else if (cmd == "quit") { PostQuitMessage(0); s_selftestActive = false; return; }
    }
    s_selftestActive = false;
}

// ---------------------------------------------------------------------------
//  ana çizim
void Draw(EditorState* es, ImGuiIO& io) {
    // selftest yükle
    if (!s_selftestPath.empty() && !s_selftestActive && s_selftestLines.empty()) {
        std::string txt;
        if (ReadFileTextA(s_selftestPath, txt)) {
            std::istringstream iss(txt);
            std::string l;
            while (std::getline(iss, l)) s_selftestLines.push_back(l);
            s_selftestActive = true;
            g_statusText = "SELFTEST AKT\304\260F";
        }
        s_selftestPath.clear();
    }
    // pending open
    if (!s_pendingOpen.empty()) {
        if (forms::LoadProject(s_pendingOpen)) es->welcome = 0;
        s_pendingOpen.clear();
    }

    if (es && es->welcome) { DrawWelcome(es, io); return; }

    SelftestStep();
    s_selftestFrame++;

    // ---- menü çubuğu ----
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Dosya")) {
            if (ImGui::MenuItem("Yeni Proje", "Ctrl+N")) DoNewProject();
            if (ImGui::MenuItem("A\303\247...", "Ctrl+O")) DoOpen();
            if (ImGui::MenuItem("Kaydet", "Ctrl+S")) DoSave(false);
            if (ImGui::MenuItem("Farkl\304\261 Kaydet...")) DoSave(true);
            if (ImGui::MenuItem("Klas\303\266r Projesi Kaydet (Forms.info)")) DoSaveFolder();
            ImGui::Separator();
            if (ImGui::MenuItem("\303\207\304\261k\305\237\304\261\305\237")) PostQuitMessage(0);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("D\303\274zen")) {
            ImGui::BeginDisabled(!g_model.CanUndo());
            if (ImGui::MenuItem("Geri Al", "Ctrl+Z")) g_model.Undo();
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!g_model.CanRedo());
            if (ImGui::MenuItem("Yinele", "Ctrl+Y")) g_model.Redo();
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::MenuItem("Sil", "Del")) { if (g_model.selected) { g_model.PushUndo(); g_model.Delete(g_model.selected); } }
            ImGui::BeginDisabled(!g_model.selected);
            if (ImGui::MenuItem("\303\226ne Getir")) g_model.BringToFront(g_model.selected);
            if (ImGui::MenuItem("Arkaya G\303\266nder")) g_model.SendToBack(g_model.selected);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("G\303\266r\303\274n\303\274m")) {
            if (ImGui::MenuItem("Izgara", nullptr, g_gridVisible)) g_gridVisible = !g_gridVisible;
            if (ImGui::MenuItem("Snap (8px)", nullptr, g_snapEnabled)) g_snapEnabled = !g_snapEnabled;
            ImGui::Separator();
            if (ImGui::MenuItem("Kod Gezgini", nullptr, g_showCodetree)) g_showCodetree = !g_showCodetree;
            if (ImGui::MenuItem("Ara\303\247 Kutusu", nullptr, g_showToolbox)) g_showToolbox = !g_showToolbox;
            if (ImGui::MenuItem("Formlar", nullptr, g_showForms)) g_showForms = !g_showForms;
            if (ImGui::MenuItem("\303\226zellikler/Olaylar", nullptr, g_showProps)) g_showProps = !g_showProps;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("\303\234retim")) {
            if (ImGui::MenuItem("Kod \303\234ret...", "F6")) s_genDialog = true;
            ImGui::MenuItem("D3D S\303\274r\303\274m\303\274 se\303\247im diyalo\304\237u", nullptr, false);
            ImGui::Separator();
            ImGui::Text("Aktif motor: %s | \303\234retim: %s",
                        BackendNameA((EditorBackend)(es ? es->editorBackend : 0)),
                        BackendNameA((EditorBackend)codegen::g_settings.backend));
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tema")) {
            for (auto& t : theme::List())
                if (ImGui::MenuItem(t.name, nullptr, s_editorTheme == t.id)) {
                    s_editorTheme = t.id;
                    theme::Apply(t.id);
                }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Yard\304\261m")) {
            if (ImGui::MenuItem("Hakk\304\261nda")) s_aboutOpen = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ---- araç çubuğu ----
    {
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 30));
        ImGui::Begin("##tb", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        if (ToolbarButton("Yeni", "Yeni proje")) DoNewProject();
        ImGui::SameLine();
        if (ToolbarButton("A\303\247", "Proje a\303\247 (Ctrl+O)")) DoOpen();
        ImGui::SameLine();
        if (ToolbarButton("Kaydet", "Kaydet (Ctrl+S)")) DoSave(false);
        ImGui::SameLine(); ImGui::Separator();
        if (ToolbarButton("Geri", "Geri al (Ctrl+Z)", g_model.CanUndo())) g_model.Undo();
        ImGui::SameLine();
        if (ToolbarButton("\304\260leri", "Yinele (Ctrl+Y)", g_model.CanRedo())) g_model.Redo();
        ImGui::SameLine(); ImGui::Separator();
        if (ToolbarButton("Sil", "Sil (Del)", g_model.selected != nullptr)) { g_model.PushUndo(); g_model.Delete(g_model.selected); }
        ImGui::SameLine(); ImGui::Separator();
        if (ToolbarButton("\303\234ret", "Kod \303\234ret (F6)")) s_genDialog = true;
        ImGui::SameLine(); ImGui::Separator();
        ImGui::Text("%s | %s | motor: %s | \303\274retim: %s",
                    forms::Active() ? forms::Active()->name.c_str() : "-",
                    g_model.selected ? g_model.selected->name.c_str() : (g_model.formSelected ? "form" : "-"),
                    BackendNameA((EditorBackend)(es ? es->editorBackend : 0)),
                    BackendNameA((EditorBackend)codegen::g_settings.backend));
        ImGui::End();
    }

    // ---- yerleşim ----
    float menuH = ImGui::GetFrameHeight() + 30.0f + 6.0f;
    float statusH = STATUSBAR_H;
    float leftW = (g_showCodetree || g_showToolbox) ? ImMax(CODETREE_W, TOOLBOX_W) : 0;
    float rightW = g_showProps ? PROPS_W : 0;
    float bodyX0 = leftW + (leftW ? SPLIT_W : 0);
    float bodyX1 = io.DisplaySize.x - (rightW ? rightW + SPLIT_W : 0);
    float bodyY0 = menuH;
    float bodyY1 = io.DisplaySize.y - statusH;

    // sol sütun
    if (leftW > 0) {
        float ctH = g_showCodetree ? (bodyY1 - bodyY0) * 0.5f - SPLIT_W / 2 : 0;
        if (g_showCodetree && ctH > 40) DrawCodetree(ImVec2(0, bodyY0), ImVec2(leftW, bodyY0 + ctH));
        if (g_showToolbox) DrawToolbox(ImVec2(0, g_showCodetree ? bodyY0 + ctH + SPLIT_W : bodyY0), ImVec2(leftW, bodyY1));
    }
    // sağ sütun
    if (g_showForms) {
        DrawFormList(ImVec2(bodyX1, bodyY0), ImVec2(io.DisplaySize.x, bodyY0 + FORMLIST_H));
        if (g_showProps) DrawPropsPanel(ImVec2(bodyX1, bodyY0 + FORMLIST_H + SPLIT_W), ImVec2(io.DisplaySize.x, bodyY1));
    } else if (g_showProps) {
        DrawPropsPanel(ImVec2(bodyX1, bodyY0), ImVec2(io.DisplaySize.x, bodyY1));
    }

    // merkez: canvas
    canvas::Draw(ImVec2(bodyX0, bodyY0), ImVec2(bodyX1, bodyY1));

    // ---- durum çubuğu ----
    {
        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - statusH));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, statusH));
        ImGui::Begin("##status", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("%s   |   %s   |   ızgara:%s snap:%s   |   %d kare",
                    s_status.c_str(),
                    g_modelDirty ? "KAYDED\304\260LMED\304\260\305\236" : "temiz",
                    g_gridVisible ? "a\303\247\304\261k" : "kapal\304\261",
                    g_snapEnabled ? "8px" : "kapal\304\261",
                    s_selftestFrame);
        ImGui::End();
    }

    // diyaloglar
    if (s_genDialog) DrawGenDialog();
    if (s_aboutOpen) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 260, io.DisplaySize.y * 0.5f - 140), ImGuiCond_FirstUseEver);
        ImGui::Begin("Hakk\304\261nda", &s_aboutOpen, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("IMGUI GOD SSS TIER v1.0");
        ImGui::TextUnformatted("M2GodWin32 Faz 2.13 evreninin ImGui d\303\266n\303\274\305\237\303\274m\303\274.");
        ImGui::TextUnformatted("Edit\303\266r: ImGui (WYSIWYG canvas) + se\303\247ilebilir D3D motoru.");
        ImGui::TextUnformatted("\303\234retim: VS2019 (v142) / MinGW — DX9/DX10/DX11/DX12/OpenGL3.");
        ImGui::TextUnformatted("Temalar: ImGui Dark/Light/Classic + GOD OPS / Midnight Steel / Ammo Tan.");
        ImGui::TextUnformatted("(Ruby/Yellow/Purple Graphite Win32API'ye \303\266zeldi — bilin\303\247li olarak YOK.)");
        ImGui::End();
    }

    HandleShortcuts(io);
}

} // namespace ui_editor
