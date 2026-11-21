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

static std::string s_status = "HAZIR";
static std::string s_selftestPath;
static std::string s_pendingOpen;
static bool s_genDialog = false;             // üretim diyaloğu açık
static bool s_aboutOpen = false;
static int s_editorTheme = theme::TImGuiDark;
static bool s_renameInit = false;      // form adlandırma popup'ı tampon bayrağı
static bool s_popupAtFrameStart = false; // kare başında popup açık mıydı (ESC yarışı için)

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
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        if (ImGui::TreeNodeEx(cat.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& e : g_catalog.catElements[cat]) {
                ImGui::PushID(e.c_str());
                // tık = YERLEŞTİRME MODU (arm — ESC iptal); çift tık = aktif forma direkt ekle
                if (ImGui::Selectable(e.c_str(), canvas::IsArmed() && canvas::g_armTool == e,
                                      ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        canvas::ArmTool(e);
                        canvas::AddArmedToForm();
                        canvas::CancelArm();
                        s_status = "Eklendi: " + e + " (aktif forma)";
                    } else {
                        canvas::ArmTool(e);
                        s_status = "Yerle\305\237tir: " + e + " — forma t\304\261kla (ESC iptal)";
                    }
                }
                if (ImGui::IsItemHovered()) {
                    const ElementDef* d = g_catalog.Find(e);
                    if (d) ImGui::SetTooltip("%s\n%s\nT\304\261kla: yerle\305\237tir · \303\247ift t\304\261kla: forma ekle",
                                             d->cppApi.c_str(), d->kind.c_str());
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

// kod gezgini satırı — SAĞLAM seçim (Selectable) + bağlam menüsü + hiyerarşi
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
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) ImGui::CloseCurrentPopup();   // nav kapalı → biz kapatırız
        if (ImGui::MenuItem("Se\303\247")) canvas::Select(c, false);
        ImGui::Separator();
        if (ImGui::MenuItem("Kopyala (Ctrl+C)", nullptr, false, g_model.IsSelected(c))) { g_model.CopySelection(); ImGui::CloseCurrentPopup(); }
        if (ImGui::MenuItem("Kes (Ctrl+X)", nullptr, false, g_model.IsSelected(c))) { g_model.PushUndo(); g_model.CutSelection(); ImGui::CloseCurrentPopup(); }
        ImGui::Separator();
        if (ImGui::MenuItem("\303\226ne Getir")) { g_model.BringToFront(c); ImGui::CloseCurrentPopup(); }
        if (ImGui::MenuItem("Arkaya G\303\266nder")) { g_model.SendToBack(c); ImGui::CloseCurrentPopup(); }
        ImGui::Separator();
        if (ImGui::MenuItem("Sil (Del)")) { g_model.PushUndo(); g_model.SelectOnly(c); g_model.DeleteList(g_model.ValidSelection()); ImGui::CloseCurrentPopup(); }
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
            // AKTİF form → g_model (düzenleme yüzeyi); park edilmiş → girdi modeli
            const Model& fm = ((int)i == forms::ActiveIndex()) ? g_model : f->model;
            for (Ctrl* c : fm.ctrls)
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

static void DrawFormList(ImVec2 min, ImVec2 max) {
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(max.x - min.x, max.y - min.y));
    ImGui::Begin("Formlar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
    for (size_t i = 0; i < forms::Count(); i++) {
        forms::FormEntry* f = forms::At(i);
        ImGui::PushID((int)i);
        size_t nObj = ((int)i == forms::ActiveIndex()) ? g_model.ctrls.size() : f->model.ctrls.size();
        std::string line = f->name + "   (" + std::to_string(nObj) + " nesne)";
        if (ImGui::Selectable(line.c_str(), (int)i == forms::ActiveIndex())) forms::SwitchTo((int)i);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            s_renameInit = false;                      // popup açılırken tampon tazelenecek
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
    if (ImGui::Button("Yen. Ad")) { s_renameInit = false; ImGui::OpenPopup("Yeniden Adland\304\261r"); }
    if (ImGui::BeginPopup("Yeniden Adland\304\261r")) {
        static char nb[128];
        // HATA DÜZELTİLDİ: tampon HER KARE yeniden dolduruluyordu → yazılan ad
        // anında geri yükleniyordu (adlandırma kullanılamaz haldeydi).
        // Artık yalnızca açılış karesinde dolar, sonra kullanıcı serbest yazar.
        if (!s_renameInit) { snprintf(nb, sizeof nb, "%s", forms::Active()->name.c_str()); s_renameInit = true; }
        // ESC = iptal (NavEnableKeyboard kapalı → ImGui kendi kapatmıyor)
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { s_renameInit = false; ImGui::CloseCurrentPopup(); }
        if (ImGui::InputText("Yeni ad", nb, sizeof nb)) {}
        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            forms::RenameActive(nb); s_renameInit = false; ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Tamam")) { forms::RenameActive(nb); s_renameInit = false; ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("\304\260ptal")) { s_renameInit = false; ImGui::CloseCurrentPopup(); }
        if (!ImGui::IsPopupOpen("Yeniden Adland\304\261r")) s_renameInit = false;   // kapandıysa sıfırla
        ImGui::EndPopup();
    }
    ImGui::End();
}

void DrawPropsPanel(ImVec2 min, ImVec2 max);   // props.cpp'de

// ---------------------------------------------------------------------------
//  klavye kısayolları
static void HandleShortcuts(ImGuiIO& io) {
    // metin girişi odaktaysa kısayolları yeme (panellerdeki InputText)
    if (io.WantTextInput) return;
    // AÇIK POPUP varsa kısayollar KAPALI (bağlam menüsü, adlandırma popup'ı...):
    // ESC popup'ı kapatmalı — arkasındaki seçimi silmemeli; ok/Del de menüye
    // karışmamalı.
    // NOT: kare İÇİNDE kapanan popup'ı da hesaba katmak için kare BAŞINDAKİ durum
    // kullanılır (ESC aynı karede popup'ı kapatır; arkasından gelen "seçimi temizle"
    // yan etkisi bu sayede tetiklenmez).
    if (s_popupAtFrameStart) return;
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
                // tuş serisi başında BİR KEZ undo kaydı (tuş atlamaları tek
                // geri alma ile toparlanır)
                static double s_lastArrowT = -10.0;
                double t = ImGui::GetTime();
                if (t - s_lastArrowT > 0.7) g_model.PushUndo();
                s_lastArrowT = t;
                for (Ctrl* c : g_model.ValidSelection()) { c->x += dx; c->y += dy; }
                g_modelDirty = true;
            }
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
// olay enjeksiyon kuyruğu (karelere yayılmış mesajlar)
struct InjectedMsg { int atFrame; UINT msg; WPARAM wp; LPARAM lp; };
static std::vector<InjectedMsg> s_inject;
static void Inject(int framesLater, UINT msg, WPARAM wp, LPARAM lp) {
    InjectedMsg m; m.atFrame = s_selftestFrame + framesLater; m.msg = msg; m.wp = wp; m.lp = lp;
    s_inject.push_back(m);
}
static void DrainInject() {
    HWND hwnd = (HWND)g_mainHwnd;
    if (!hwnd) return;
    for (size_t i = 0; i < s_inject.size(); ) {
        if (s_inject[i].atFrame <= s_selftestFrame) {
            PostMessageA(hwnd, s_inject[i].msg, s_inject[i].wp, s_inject[i].lp);
            s_inject.erase(s_inject.begin() + i);
        } else i++;
    }
}
static void SelftestStep() {
    if (!s_selftestActive) return;
    DrainInject();
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
        } else if (cmd == "selectadd") {
            std::string n; iss >> n;
            Ctrl* c = g_model.FindByName(n);
            if (c) g_model.AddSel(c);
        } else if (cmd == "set") {
            std::string n, p; iss >> n >> p;
            std::string rest; std::getline(iss, rest);
            rest = TrimA(rest);
            // tırnak içi değeri soy: "#D25410" → #D25410
            if (rest.size() >= 2 && (rest.front() == '"' || rest.front() == '\'') && rest.back() == rest.front())
                rest = rest.substr(1, rest.size() - 2);
            Ctrl* c = g_model.FindByName(n);
            if (c) {
                if (p == "Name") g_model.Rename(c, rest);
                else {
                    c->props[p] = rest;
                    // WYSIWYG senkronu: boyut prop'ları tasarım boyutunu da günceller
                    if (p == "Width" || p == "SizeArgW") { int v = atoi(rest.c_str()); if (v > 0) c->w = v; }
                    if (p == "Height" || p == "SizeArgH") { int v = atoi(rest.c_str()); if (v > 0) c->h = v; }
                    // ListBox satır sayısı ↔ tasarım yüksekliği (görselle tutarlı)
                    if (p == "HeightItems") {
                        int items = atoi(rest.c_str());
                        if (items >= 1)
                            c->h = (int)(ImGui::GetTextLineHeightWithSpacing() * (items + 0.25f)
                                         + ImGui::GetStyle().FramePadding.y * 2.0f + 0.5f);
                    }
                }
            }
        } else if (cmd == "form") { iss >> g_model.formW >> g_model.formH; g_model.formProps["ClientSize"] = std::to_string(g_model.formW) + ", " + std::to_string(g_model.formH); }
        else if (cmd == "newform") { forms::NewForm(); }
        else if (cmd == "save") { std::string p; std::getline(iss, p); p = TrimA(p); forms::SaveProjectAs(p); }
        else if (cmd == "savefolder") { std::string p; std::getline(iss, p); p = TrimA(p); forms::SaveFolderProject(p); }
        else if (cmd == "gen") { std::string p; std::getline(iss, p); p = TrimA(p); codegen::EmitProject(p); }
        else if (cmd == "theme") { int t; iss >> t; theme::Apply(t); }
        else if (cmd == "genbackend") { int b; iss >> b; codegen::g_settings.backend = b; }
        else if (cmd == "projname") { std::string n; iss >> n; codegen::g_settings.projectName = n; }
        else if (cmd == "wait") { int k; iss >> k; s_selftestWaitUntil = s_selftestFrame + k; return; }
        else if (cmd == "arm") { std::string t; iss >> t; canvas::ArmTool(t); s_status = "ARM: " + t; }
        else if (cmd == "click") {
            // gerçek tıklama enjeksiyonu (ImGui Win32 backend'in işlediği WM mesajları)
            int x, y; iss >> x >> y;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd) {
                PostMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
                s_status = "click " + std::to_string(x) + "," + std::to_string(y);
            }
        }
        else if (cmd == "dblclick") {
            int x, y; iss >> x >> y;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd) {
                PostMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
            }
        }
        else if (cmd == "drag") {
            // rubber band / nesne taşıma sürüklemesi — MESAJLAR KARELERE YAYILIR
            // (tek karede down+up = tık olarak birleşir; sürükleme = ayrı kareler)
            int x1, y1, x2, y2; iss >> x1 >> y1 >> x2 >> y2;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd) {
                Inject(0, WM_MOUSEMOVE, 0, MAKELPARAM(x1, y1));
                Inject(0, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x1, y1));
                Inject(1, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + (x2 - x1) / 3, y1 + (y2 - y1) / 3));
                Inject(2, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + 2 * (x2 - x1) / 3, y1 + 2 * (y2 - y1) / 3));
                Inject(3, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x2, y2));
                Inject(4, WM_LBUTTONUP, 0, MAKELPARAM(x2, y2));
            }
        }
        else if (cmd == "key") {
            // key <ad> [ctrl] — ad: a-z, del, esc, left/right/up/down, enter, f5, f6
            std::string k; int ctrl = 0; iss >> k >> ctrl;
            UINT vk = 0;
            if (k.size() == 1 && k[0] >= 'a' && k[0] <= 'z') vk = (UINT)(k[0] - 'a' + 'A');
            else if (k == "del" || k == "delete") vk = VK_DELETE;
            else if (k == "esc" || k == "escape") vk = VK_ESCAPE;
            else if (k == "left") vk = VK_LEFT;
            else if (k == "right") vk = VK_RIGHT;
            else if (k == "up") vk = VK_UP;
            else if (k == "down") vk = VK_DOWN;
            else if (k == "enter") vk = VK_RETURN;
            else if (k == "f5") vk = VK_F5;
            else if (k == "f6") vk = VK_F6;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd && vk) {
                if (ctrl) { PostMessageA(hwnd, WM_KEYDOWN, VK_CONTROL, 0x1D0001); }
                PostMessageA(hwnd, WM_KEYDOWN, vk, 0);
                PostMessageA(hwnd, WM_KEYUP, vk, 0);
                if (ctrl) { PostMessageA(hwnd, WM_KEYUP, VK_CONTROL, 0x1D0001); }
            }
        }
        else if (cmd == "paste") { g_model.PasteClipboard(GRID_SIZE, GRID_SIZE); }
        else if (cmd == "copy") { g_model.CopySelection(); }
        else if (cmd == "cut") { g_model.CutSelection(); }
        else if (cmd == "selectall") { g_model.SelectAll(); }
        else if (cmd == "selcount") {
            // seçim sayısını status'a yaz — test doğrulaması
            s_status = "SEL:" + std::to_string(g_model.selList.size()) +
                       " CLIP:" + std::to_string(!g_model.clipboardJson.empty());
        }
        else if (cmd == "rclick") {
            // sağ tık enjeksiyonu (bağlam menüsü / seçim testleri)
            int x, y; iss >> x >> y;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd) {
                PostMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_RBUTTONUP, 0, MAKELPARAM(x, y));
                s_status = "rclick " + std::to_string(x) + "," + std::to_string(y);
            }
        }
        else if (cmd == "rclickobj") {
            // rclickobj <isim> — nesnenin GERÇEK çizim dikdörtgeni ortasına sağ tık
            std::string n; iss >> n;
            HWND hwnd = (HWND)g_mainHwnd;
            float rx, ry, rw, rh;
            if (hwnd && canvas::ObjRectOf(n, &rx, &ry, &rw, &rh)) {
                int cx = (int)(rx + rw * 0.5f), cy = (int)(ry + rh * 0.5f);
                Inject(0, WM_MOUSEMOVE, 0, MAKELPARAM(cx, cy));
                Inject(0, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(cx, cy));
                Inject(1, WM_RBUTTONUP, 0, MAKELPARAM(cx, cy));
                s_status = "rclickobj " + n;
            }
        }
        else if (cmd == "dragform") {
            // dragform <dx> <dy> — formun GERÇEK sağ-alt köşe tutamacından sürükleme
            // (form boyutlandırma birikim hatası testi: sonuç kesin delta olmalı)
            int dx, dy; iss >> dx >> dy;
            HWND hwnd = (HWND)g_mainHwnd;
            float fx, fy, fw, fh;
            canvas::GetFormRect(&fx, &fy, &fw, &fh);
            if (hwnd) {
                int x1 = (int)(fx + fw), y1 = (int)(fy + fh);
                int x2 = x1 + dx, y2 = y1 + dy;
                Inject(0, WM_MOUSEMOVE, 0, MAKELPARAM(x1, y1));
                Inject(0, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x1, y1));
                Inject(1, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + dx / 3, y1 + dy / 3));
                Inject(2, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + 2 * dx / 3, y1 + 2 * dy / 3));
                Inject(3, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x2, y2));
                Inject(4, WM_LBUTTONUP, 0, MAKELPARAM(x2, y2));
                s_status = "dragform " + std::to_string(dx) + "," + std::to_string(dy);
            }
        }
        else if (cmd == "wheel") {
            // wheel <x> <y> <delta> — fare tekerleği (panel kaydırma testleri)
            int x, y, d; iss >> x >> y >> d;
            HWND hwnd = (HWND)g_mainHwnd;
            if (hwnd) {
                PostMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                PostMessageA(hwnd, WM_MOUSEWHEEL, (WPARAM)(d << 16), MAKELPARAM(x, y));
                s_status = "wheel " + std::to_string(d);
            }
        }
        else if (cmd == "snap") {
            // snap on|off — ızgara yuvarlama anahtarını çevir (test hassasiyeti)
            std::string v; iss >> v;
            g_snapEnabled = (v != "off");
        }
        else if (cmd == "statusfile") {
            // statusfile <yol> — durum metnini dosyaya yaz (test doğrulama kancası)
            std::string p; std::getline(iss, p);
            p = TrimA(p);
            if (!p.empty()) WriteFileTextA(p, s_status + "\n");
        }
        else if (cmd == "dragobj") {
            // dragobj <isim> <dx> <dy> — nesnenin SAĞ kenar tutamacından sürükleme
            // (yalnız-genişlik tiplerde sağ/sol tutamaç vardır — maskeli boyutlandırma testi)
            std::string n; int dx, dy; iss >> n >> dx >> dy;
            HWND hwnd = (HWND)g_mainHwnd;
            float rx, ry, rw, rh;
            if (hwnd && canvas::ObjRectOf(n, &rx, &ry, &rw, &rh)) {
                int x1 = (int)(rx + rw), y1 = (int)(ry + rh * 0.5f);   // sağ kenar ortası (tutamaç 3)
                Inject(0, WM_MOUSEMOVE, 0, MAKELPARAM(x1, y1));
                Inject(0, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x1, y1));
                Inject(1, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + dx / 2, y1 + dy / 2));
                Inject(3, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1 + dx, y1 + dy));
                Inject(4, WM_LBUTTONUP, 0, MAKELPARAM(x1 + dx, y1 + dy));
                s_status = "dragobj " + n;
            }
        }
        else if (cmd == "objsize") {
            // objsize <isim> [dosya] — nesnenin GERÇEK ÇİZİM boyutunu (WxH) yazar.
            // Test kancası: özellikteki Boyut ile görsel boyut aynı mı? (WYSIWYG)
            std::string n, p; iss >> n;
            std::getline(iss, p); p = TrimA(p);
            float rx, ry, rw, rh;
            if (canvas::ObjRectOf(n, &rx, &ry, &rw, &rh)) {
                char buf[128];
                snprintf(buf, sizeof buf, "%s:%dx%d\n", n.c_str(), (int)(rw + 0.5f), (int)(rh + 0.5f));
                s_status = buf;
                if (!p.empty()) {
                    FILE* f = fopen(p.c_str(), "a");
                    if (f) { fputs(buf, f); fclose(f); }
                }
            } else {
                s_status = "objsize: bulunamadi " + n;
            }
        }
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

    // kare başındaki popup durumu — HandleShortcuts yarış koruması için
    s_popupAtFrameStart = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

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
            if (ImGui::MenuItem("Sil", "Del")) { if (!g_model.selList.empty()) { g_model.PushUndo(); g_model.DeleteList(g_model.ValidSelection()); } }
            ImGui::BeginDisabled(g_model.selList.empty());
            if (ImGui::MenuItem("\303\226ne Getir")) g_model.BringToFront(g_model.Primary());
            if (ImGui::MenuItem("Arkaya G\303\266nder")) g_model.SendToBack(g_model.Primary());
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
        if (ToolbarButton("Sil", "Sil (Del)", !g_model.selList.empty())) { g_model.PushUndo(); g_model.DeleteList(g_model.ValidSelection()); }
        ImGui::SameLine(); ImGui::Separator();
        if (ToolbarButton("\303\234ret", "Kod \303\234ret (F6)")) s_genDialog = true;
        ImGui::SameLine(); ImGui::Separator();
        ImGui::Text("%s | %s | motor: %s | \303\274retim: %s",
                    forms::Active() ? forms::Active()->name.c_str() : "-",
                    g_model.selList.size() > 1 ? (std::to_string(g_model.selList.size()) + " nesne").c_str() : (g_model.Primary() ? g_model.Primary()->name.c_str() : (g_model.formSelected ? "form" : "-")),
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
