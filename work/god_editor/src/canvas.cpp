// ============================================================================
//  canvas.cpp — WYSIWYG tasarım yüzeyi.
//  Editördeki form, GERÇEK bir ImGui penceresi olarak çizilir; içindeki her
//  nesne üretilen kodla AYNI ImGui çağrılarıyla render edilir. Seçim, 8
//  boyutlandırma tutamacı, ızgara snap'i, sürükleyip bırakma.
// ============================================================================
#include "canvas.h"
#include "ui_editor.h"
#include "forms.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "imgui_internal.h"

namespace canvas {

std::string g_armTool;
Ctrl* g_armParent = nullptr;

// ---------------------------------------------------------------------------
//  durum
static Ctrl* s_sel = nullptr;         // canvas'ın kendi seçim aynası (g_model.selected ile senkron)
static bool  s_dragBody = false;
static int   s_dragHandle = -1;       // -1 yok; 0..7 (saat yönünde: 0 sol üst, 1 üst-orta ...)
static ImVec2 s_dragStart, s_dragObjStart;
static int s_origX, s_origY, s_origW, s_origH;
static ImVec2 s_canvasMin, s_canvasMax;

void Select(Ctrl* c) {
    g_model.selected = c;
    g_model.formSelected = (c == nullptr);
    s_sel = c;
}
void SelectForm() { Select(nullptr); }

ImVec2 CtrlAbsPos(const Ctrl* c) {
    int x = c->x, y = c->y;
    for (Ctrl* p = c->parent; p; p = p->parent) { x += p->x; y += p->y; }
    return ImVec2((float)x, (float)y);
}

// ---------------------------------------------------------------------------
//  nesne çizimi — üretilen kodla BİREBİR aynı ImGui dizisi
//  (parite testi: tests/parity_test.cpp canvas çizimini kod üretimiyle karşılaştırır)
struct WidgetRect { Ctrl* c; ImVec2 rmin, rmax; };
static std::vector<WidgetRect> s_hitRects;    // bu karede taranabilir yüzeyler

static void PushRect(const Ctrl* c) {
    WidgetRect r; r.c = c;
    r.rmin = ImGui::GetItemRectMin();
    r.rmax = ImGui::GetItemRectMax();
    s_hitRects.push_back(r);
}

static void DrawCtrl(const Ctrl* c, const Model& m) {
    const std::string& t = c->type;
    const ImVec2 pos((float)c->x, (float)c->y);
    if (t == "Button") {
        int w = atoi(c->Get("SizeArgW", "120").c_str());
        int h = atoi(c->Get("SizeArgH", "28").c_str());
        ImVec2 sz((float)(c->w > 0 ? c->w : w), (float)(c->h > 0 ? c->h : h));
        ImGui::SetCursorPos(pos);
        ImGui::Button(c->Get("Text", "Buton").c_str(), sz);
        PushRect(c);
    } else if (t == "Checkbox") {
        // tasarım değerini simüle: yerel bool
        bool v = c->GetB("Checked");
        ImGui::SetCursorPos(pos);
        ImGui::Checkbox(c->Get("Text", "Seçenek").c_str(), &v);
        PushRect(c);
    } else if (t == "RadioButton") {
        bool v = c->GetB("Active");
        ImGui::SetCursorPos(pos);
        ImGui::RadioButton(c->Get("Text", "Seçim").c_str(), &v);
        PushRect(c);
    } else if (t == "SliderFloat" || t == "SliderInt") {
        float fv = (float)atof(c->Get("Value", "0").c_str());
        int   iv = atoi(c->Get("Value", "0").c_str());
        float mn = (float)atof(c->Get("Min", "0").c_str());
        float mx = (float)atof(c->Get("Max", "100").c_str());
        const char* fmt = c->Get("Format", t == "SliderFloat" ? "%.1f" : "%d").c_str();
        ImGui::SetCursorPos(pos);
        ImVec2 sz((float)c->w, (float)c->h);
        if (t == "SliderFloat") ImGui::SliderFloat(c->Get("Text", "Değer").c_str(), &fv, mn, mx, fmt);
        else ImGui::SliderInt(c->Get("Text", "Değer").c_str(), &iv, (int)mn, (int)mx, fmt);
        PushRect(c);
    } else if (t == "DragFloat" || t == "DragInt") {
        float fv = (float)atof(c->Get("Value", "0").c_str());
        int   iv = atoi(c->Get("Value", "0").c_str());
        float mn = (float)atof(c->Get("Min", "0").c_str());
        float mx = (float)atof(c->Get("Max", "100").c_str());
        ImGui::SetCursorPos(pos);
        if (t == "DragFloat") ImGui::DragFloat(c->Get("Text", "Değer").c_str(), &fv, 1.0f, mn, mx);
        else ImGui::DragInt(c->Get("Text", "Değer").c_str(), &iv, 1.0f, (int)mn, (int)mx);
        PushRect(c);
    } else if (t == "InputText" || t == "InputTextMultiline") {
        static char buf[64]; // tasarım değeri gösterilir (yalnızca görünüm)
        snprintf(buf, sizeof buf, "%s", c->Get("Value", "").c_str());
        ImGui::SetCursorPos(pos);
        if (t == "InputText") ImGui::InputText(c->Get("Text", "Yazı").c_str(), buf, sizeof buf, ImGuiInputTextFlags_ReadOnly);
        else {
            ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "280").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "120").c_str())));
            ImGui::InputTextMultiline(c->Get("Text", "Not").c_str(), buf, sizeof buf, sz, ImGuiInputTextFlags_ReadOnly);
        }
        PushRect(c);
    } else if (t == "InputInt" || t == "InputFloat") {
        int iv = atoi(c->Get("Value", "0").c_str());
        float fv = (float)atof(c->Get("Value", "0").c_str());
        ImGui::SetCursorPos(pos);
        if (t == "InputInt") ImGui::InputInt(c->Get("Text", "Sayı").c_str(), &iv, atoi(c->Get("Step", "1").c_str()));
        else ImGui::InputFloat(c->Get("Text", "Sayı").c_str(), &fv, (float)atof(c->Get("Step", "1").c_str()));
        PushRect(c);
    } else if (t == "Combo" || t == "ListBox") {
        static int sel; sel = atoi(c->Get("Selected", "0").c_str());
        auto items = SplitW(Utf8ToWideStr(c->Get("Items", "")), L'|');
        std::vector<const char*> its;
        for (auto& i : items) its.push_back(nullptr); // dolgu — aşağıda ömür sorununu önle
        // ömür güvenli: ayrı string deposu
        static std::vector<std::string> s_store;
        s_store.clear();
        for (auto& i : items) s_store.push_back(WideStrToUtf8(i));
        std::vector<const char*> ptrs;
        for (auto& s : s_store) ptrs.push_back(s.c_str());
        const char* cur = (sel >= 0 && sel < (int)ptrs.size()) ? ptrs[sel] : "";
        ImGui::SetCursorPos(pos);
        if (t == "Combo") {
            if (ImGui::BeginCombo(c->Get("Text", "Seçim").c_str(), cur)) ImGui::EndCombo();
            PushRect(c);
        } else {
            if (ImGui::ListBoxHeader(c->Get("Text", "Liste").c_str(), atoi(c->Get("HeightItems", "5").c_str()))) {
                for (auto& p : ptrs) ImGui::Selectable(p, false);
                ImGui::ListBoxFooter();
            }
            PushRect(c);
        }
    } else if (t == "ColorEdit4") {
        ImVec4 col(atof(c->Get("R", "255").c_str()) / 255.0f,
                    atof(c->Get("G", "255").c_str()) / 255.0f,
                    atof(c->Get("B", "255").c_str()) / 255.0f, 1.0f);
        ImGui::SetCursorPos(pos);
        ImGui::ColorEdit4(c->Get("Text", "Renk").c_str(), &col.x, ImGuiColorEditFlags_NoInputs);
        PushRect(c);
    } else if (t == "ProgressBar") {
        ImGui::SetCursorPos(pos);
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "200").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "18").c_str())));
        ImGui::ProgressBar((float)atof(c->Get("Value", "0.5").c_str()), sz, c->Get("Overlay", "").c_str()[0] ? c->Get("Overlay", "").c_str() : nullptr);
        PushRect(c);
    } else if (t == "Separator") {
        ImGui::SetCursorPos(pos);
        ImGui::Separator();
        PushRect(c);
    } else if (t == "SeparatorText") {
        ImGui::SetCursorPos(pos);
        ImGui::SeparatorText(c->Get("Text", "Bölüm").c_str());
        PushRect(c);
    } else if (t == "Text" || t == "TextColored" || t == "BulletText" || t == "LabelText") {
        ImGui::SetCursorPos(pos);
        if (t == "Text") ImGui::Text("%s", c->Get("Text", "Etiket").c_str());
        else if (t == "TextColored") {
            ImVec4 col(atof(c->Get("R", "255").c_str()) / 255.0f,
                        atof(c->Get("G", "200").c_str()) / 255.0f,
                        atof(c->Get("B", "80").c_str()) / 255.0f, 1.0f);
            ImGui::TextColored(col, "%s", c->Get("Text", "Renkli etiket").c_str());
        }
        else if (t == "BulletText") ImGui::BulletText("%s", c->Get("Text", "Madde").c_str());
        else ImGui::LabelText(c->Get("Text", "Alan").c_str(), "%s", c->Get("Value", "değer").c_str());
        PushRect(c);
    } else if (t == "PlotLines" || t == "PlotHistogram") {
        ImGui::SetCursorPos(pos);
        std::vector<float> data;
        for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "")), L'|')) data.push_back((float)wcstod(s.c_str(), nullptr));
        if (data.empty()) data.push_back(0.0f);
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "280").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "120").c_str())));
        if (t == "PlotLines")
            ImGui::PlotLines(c->Get("Text", "Grafik").c_str(), data.data(), (int)data.size(), 0, nullptr,
                             (float)atof(c->Get("ScaleMin", "0").c_str()), (float)atof(c->Get("ScaleMax", "1").c_str()), sz);
        else
            ImGui::PlotHistogram(c->Get("Text", "Histogram").c_str(), data.data(), (int)data.size(), 0, nullptr, FLT_MAX, FLT_MAX, sz);
        PushRect(c);
    } else if (t == "ChildWindow") {
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "300").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "200").c_str()));
        ImGui::SetCursorPos(pos);
        ImGuiChildFlags cf = c->GetB("Border") ? ImGuiChildFlags_Borders : 0;
        if (ImGui::BeginChild(c->Get("Text", "Child").c_str(), sz, cf)) {
            for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) DrawCtrl(ch, m);
        }
        ImGui::EndChild();
        PushRect(c);
    } else if (t == "GroupBox") {
        // üretilen IGOD::GroupBox ile aynı görünüm: çerçeveli child + üst başlık
        ImVec2 sz((float)(c->w ? c->w : atoi(c->Get("Width", "320").c_str())), (float)(c->h ? c->h : atoi(c->Get("Height", "160").c_str())));
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        std::string id = c->Get("Text", "Grup") + "_grp";
        ImGui::BeginChild(id.c_str(), sz, ImGuiChildFlags_Borders);
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) DrawCtrl(ch, m);
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(rmin, rmax, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
        std::string txt = c->Get("Text", "Grup");
        ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
        ImVec2 lp = ImVec2(rmin.x + 8, rmin.y - ts.y * 0.5f);
        dl->AddRectFilled(ImVec2(lp.x - 4, lp.y - 1), ImVec2(lp.x + ts.x + 4, lp.y + ts.y + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));
        dl->AddText(lp, ImGui::GetColorU32(ImGuiCol_Text), txt.c_str());
        PushRect(c);
    } else if (t == "CollapsingHeader" || t == "TreeNode") {
        ImGui::SetCursorPos(pos);
        ImGuiTreeNodeFlags fl = c->GetB("DefaultOpen") ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        bool open = (t == "CollapsingHeader") ? ImGui::CollapsingHeader(c->Get("Text", "Başlık").c_str(), fl)
                                              : ImGui::TreeNodeEx(c->Get("Text", "Düğüm").c_str(), fl);
        if (open) {
            for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) DrawCtrl(ch, m);
            if (t == "TreeNode") ImGui::TreePop();
        }
        PushRect(c);
    } else if (t == "TabBar") {
        ImGui::SetCursorPos(pos);
        std::string id = c->Get("Text", "Sekmeler") + "_tb";
        if (ImGui::BeginTabBar(id.c_str(), ImGuiTabBarFlags_None)) {
            for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Genel|Ayarlar")), L'|')) {
                std::string title = WideStrToUtf8(s);
                if (ImGui::BeginTabItem(title.c_str())) {
                    for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) {
                        if (ch->Get("TabOwner", "") == title) DrawCtrl(ch, m);
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        PushRect(c);
    } else if (t == "Table") {
        ImGui::SetCursorPos(pos);
        std::string id = c->Get("Text", "Tablo") + "_tbl";
        int cols = atoi(c->Get("Columns", "3").c_str());
        int rows = atoi(c->Get("Rows", "3").c_str());
        std::vector<std::string> heads;
        for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Sütun1|Sütun2|Sütun3")), L'|'))
            heads.push_back(WideStrToUtf8(s));
        if (ImGui::BeginTable(id.c_str(), cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            for (auto& h : heads) ImGui::TableSetupColumn(h.c_str());
            ImGui::TableHeadersRow();
            for (int r = 0; r < rows; r++) {
                ImGui::TableNextRow();
                for (int ci = 0; ci < cols; ci++) { ImGui::TableNextColumn(); ImGui::TextUnformatted("-"); }
            }
            ImGui::EndTable();
        }
        PushRect(c);
    } else if (t == "Menu") {
        // form penceresinin üstünde menü çubuğu olarak render — Begin'le_MenuBar bayrağıyla
        // (yerleşim: canvas bu nesneyi form bayraklarına ekler; çizim Form çiziminde)
    } else if (t == "Popup" || t == "Timer" || t == "FontRef") {
        // görsel değil — bileşen tepsisinde temsil edilecek; canvas'ta simge kutusu
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(("[ " + c->type + " : " + c->name + " ]").c_str());
        ImGui::PopStyleColor();
        PushRect(c);
    }
}

// ---------------------------------------------------------------------------
//  yerleştirme tıklaması / seçim tıklaması
void ClickSelectUnder(ImVec2 pt) {
    // en üstteki (son çizilen) eşleşme
    for (int i = (int)s_hitRects.size() - 1; i >= 0; i--) {
        WidgetRect& r = s_hitRects[i];
        if (pt.x >= r.rmin.x && pt.x <= r.rmax.x && pt.y >= r.rmin.y && pt.y <= r.rmax.y) {
            Select(r.c);
            return;
        }
    }
    SelectForm();
}

static bool HandleHit(ImVec2 pt, ImVec2 rmin, ImVec2 rmax, float hs) {
    // 0:sol üst 1:üst-orta 2:sağ üst 3:sağ-orta 4:sağ alt 5:alt-orta 6:sol alt 7:sol-orta
    float l = rmin.x, t = rmin.y, r = rmax.x, b = rmax.y;
    ImVec2 c[8] = { {l,t},{(l+r)/2,t},{r,t},{r,(t+b)/2},{r,b},{(l+r)/2,b},{l,b},{l,(t+b)/2} };
    for (int i = 0; i < 8; i++) {
        if (fabsf(pt.x - c[i].x) <= hs && fabsf(pt.y - c[i].y) <= hs) return true;
    }
    return false;
}

static int HandleIndexAt(ImVec2 pt, ImVec2 rmin, ImVec2 rmax, float hs) {
    float l = rmin.x, t = rmin.y, r = rmax.x, b = rmax.y;
    ImVec2 c[8] = { {l,t},{(l+r)/2,t},{r,t},{r,(t+b)/2},{r,b},{(l+r)/2,b},{l,b},{l,(t+b)/2} };
    for (int i = 0; i < 8; i++)
        if (fabsf(pt.x - c[i].x) <= hs && fabsf(pt.y - c[i].y) <= hs) return i;
    return -1;
}

// ---------------------------------------------------------------------------
//  ana çizim
void Draw(const ImVec2& rectMin, const ImVec2& rectMax) {
    s_canvasMin = rectMin; s_canvasMax = rectMax;
    s_hitRects.clear();
    Model& m = g_model;

    // canvas zemin
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(rectMin, rectMax, IM_COL32(38, 40, 48, 255));
    // ızgara
    if (g_gridVisible) {
        ImU32 dot = IM_COL32(90, 94, 110, 200);
        for (float x = rectMin.x; x < rectMax.x; x += GRID_SIZE)
            for (float y = rectMin.y; y < rectMax.y; y += GRID_SIZE)
                dl->AddCircleFilled(ImVec2(x, y), 0.7f, dot, 4);
    }

    // tasarlanan form — GERÇEK ImGui penceresi (canvas alanıyla kısıtlanmış)
    std::string title = m.formProps.count("Text") ? m.formProps["Text"] : "Form1";
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    // Flags prop'ları üretilen kodla aynı set
    auto it = m.formProps.find("Flags");
    if (it != m.formProps.end())
        for (auto& s : SplitW(Utf8ToWideStr(it->second), L'|')) {
            std::string t = TrimA(WideStrToUtf8(s));
            if (t == "NoTitleBar") flags |= ImGuiWindowFlags_NoTitleBar;
            else if (t == "NoResize") flags |= ImGuiWindowFlags_NoResize;
            else if (t == "NoCollapse") flags |= ImGuiWindowFlags_NoCollapse;
            else if (t == "NoMove") flags |= ImGuiWindowFlags_NoMove;
            else if (t == "AlwaysAutoResize") flags |= ImGuiWindowFlags_AlwaysAutoResize;
            else if (t == "NoScrollbar") flags |= ImGuiWindowFlags_NoScrollbar;
            else if (t == "MenuBar") flags |= ImGuiWindowFlags_MenuBar;
        }
    // form menüsü varsa MenuBar bayrağı
    for (Ctrl* c : m.ctrls) if (c->type == "Menu") { flags |= ImGuiWindowFlags_MenuBar; break; }

    ImVec2 formPos(rectMin.x + 40.0f, rectMin.y + 30.0f);
    ImGui::SetNextWindowPos(formPos);
    ImGui::SetNextWindowSize(ImVec2((float)m.formW, (float)m.formH));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(48, 50, 58, 255));   // tasarım zemini hafif farklı
    bool open = ImGui::Begin(title.c_str(), nullptr, flags);
    ImGui::PopStyleColor();
    if (open) {
        // menü (varsa) — üretilen kodla aynı Menus/Commands mantığı
        for (Ctrl* c : m.ctrls) if (c->type == "Menu") {
            if (ImGui::BeginMenuBar()) {
                for (auto& s : SplitW(Utf8ToWideStr(c->Get("Items", "Dosya|Düzen|Yardım")), L'|')) {
                    std::string men = WideStrToUtf8(s);
                    if (ImGui::BeginMenu(men.c_str())) {
                        for (auto& cmd : SplitW(Utf8ToWideStr(c->Get("Commands", "")), L'|')) {
                            std::string cline = WideStrToUtf8(cmd);
                            size_t col = cline.find(':');
                            if (col == std::string::npos) continue;
                            std::string owner = TrimA(cline.substr(0, col));
                            std::string item = TrimA(cline.substr(col + 1));
                            if (owner == men && !item.empty()) ImGui::MenuItem(item.c_str());
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenuBar();
            }
            // menü çerçevesini seçim dikdörtgeni olarak ekle
        }
        // üst-seviye nesneler (parent=null) — z sırası listedeki sıra
        std::vector<Ctrl*> tops;
        for (Ctrl* c : m.ctrls) if (!c->parent) tops.push_back(c);
        for (Ctrl* c : tops) DrawCtrl(c, m);
    }
    ImGui::End();

    // bileşen tepsisi (görsel olmayan) — form altında
    {
        float trayY = formPos.y + m.formH + 14.0f;
        float trayX = formPos.x;
        dl->AddRectFilled(ImVec2(trayX, trayY), ImVec2(trayX + m.formW, trayY + 26), IM_COL32(33, 35, 42, 255), 3.0f);
        dl->AddText(ImVec2(trayX + 6, trayY + 6), IM_COL32(150, 152, 160, 255), "Bile\305\237enler:");
        float cx = trayX + 90;
        for (Ctrl* c : m.ctrls) {
            if (c->type == "Timer" || c->type == "FontRef") {
                std::string lbl = "[ " + c->name + " ]";
                ImVec2 ts = ImGui::CalcTextSize(lbl.c_str());
                dl->AddRectFilled(ImVec2(cx, trayY + 3), ImVec2(cx + ts.x + 10, trayY + 23), IM_COL32(55, 58, 68, 255), 3.0f);
                dl->AddText(ImVec2(cx + 5, trayY + 6), IM_COL32(200, 202, 210, 255), lbl.c_str());
                // tıklanabilir alan kaydı
                WidgetRect r; r.c = c;
                r.rmin = ImVec2(cx, trayY + 3); r.rmax = ImVec2(cx + ts.x + 10, trayY + 23);
                s_hitRects.push_back(r);
                cx += ts.x + 16;
            }
        }
    }

    // ---- seçim vurgusu + tutamaçlar ----
    ImGuiIO& io = ImGui::GetIO();
    if (s_sel || g_model.formSelected) {
        ImVec2 rmin, rmax;
        if (s_sel) {
            // seçilen nesnenin bu karedeki gerçek çizim dikdörtgenini bul
            bool found = false;
            for (auto& r : s_hitRects) if (r.c == s_sel) { rmin = r.rmin; rmax = r.rmax; found = true; break; }
            if (!found) { rmin = formPos + CtrlAbsPos(s_sel); rmax = rmin; }   // kaba yedek
        } else {
            rmin = ImGui::GetItemRectMin();   // form: son End() — boşsa form kendisi
            rmin = formPos;
            rmax = ImVec2(formPos.x + m.formW, formPos.y + m.formH);
        }
        ImU32 accent = IM_COL32(255, 176, 32, 255);
        dl->AddRect(ImVec2(rmin.x - 2, rmin.y - 2), ImVec2(rmax.x + 2, rmax.y + 2), accent, 0.0f, 0, 1.5f);
        float hs = 4.5f;
        ImVec2 c[8] = { {rmin.x,rmin.y},{(rmin.x+rmax.x)/2,rmin.y},{rmax.x,rmin.y},{rmax.x,(rmin.y+rmax.y)/2},
                        {rmax.x,rmax.y},{(rmin.x+rmax.x)/2,rmax.y},{rmin.x,rmax.y},{rmin.x,(rmin.y+rmax.y)/2} };
        for (int i = 0; i < 8; i++) {
            dl->AddRectFilled(ImVec2(c[i].x - hs, c[i].y - hs), ImVec2(c[i].x + hs, c[i].y + hs), accent);
            dl->AddRect(ImVec2(c[i].x - hs, c[i].y - hs), ImVec2(c[i].x + hs, c[i].y + hs), IM_COL32(0, 0, 0, 255));
        }

        // ---- etkileşim: taşıma / boyutlandırma ----
        ImVec2 mp = io.MousePos;
        bool hovered = mp.x >= rmin.x - SEL_MARGIN && mp.x <= rmax.x + SEL_MARGIN && mp.y >= rmin.y - SEL_MARGIN && mp.y <= rmax.y + SEL_MARGIN;

        if (ImGui::IsMouseClicked(0) && hovered) {
            int hi = HandleIndexAt(mp, rmin, rmax, hs + 2.5f);
            if (s_sel && hi >= 0) {
                s_dragHandle = hi;
                g_model.PushUndo();
                s_origX = s_sel->x; s_origY = s_sel->y; s_origW = s_sel->w; s_origH = s_sel->h;
                s_dragStart = mp;
            } else {
                s_dragBody = true;
                if (s_sel) {
                    g_model.PushUndo();
                    s_origX = s_sel->x; s_origY = s_sel->y;
                } else {
                    g_model.PushUndo();
                    s_origX = m.formW; s_origY = m.formH;
                }
                s_dragStart = mp;
            }
        }
        if (s_dragBody && ImGui::IsMouseReleased(0)) s_dragBody = false;
        if (s_dragHandle >= 0 && ImGui::IsMouseReleased(0)) s_dragHandle = -1;

        if (s_dragBody && ImGui::IsMouseDown(0)) {
            ImVec2 d(mp.x - s_dragStart.x, mp.y - s_dragStart.y);
            if (s_sel) {
                int nx = s_origX + (int)d.x, ny = s_origY + (int)d.y;
                if (g_snapEnabled) { nx = (nx + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE; ny = (ny + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE; }
                s_sel->x = nx; s_sel->y = ny;
                g_modelDirty = true;
            } else {
                int nw = s_origX + (int)d.x, nh = s_origY + (int)d.y;
                if (g_snapEnabled) { nw = (nw + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE; nh = (nh + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE; }
                m.formW = ImMax(nw, 120); m.formH = ImMax(nh, 80);
                m.formProps["ClientSize"] = std::to_string(m.formW) + ", " + std::to_string(m.formH);
                g_modelDirty = true;
            }
        }
        if (s_dragHandle >= 0 && ImGui::IsMouseDown(0) && s_sel) {
            ImVec2 d(mp.x - s_dragStart.x, mp.y - s_dragStart.y);
            int nx = s_sel->x, ny = s_sel->y, nw = s_origW, nh = s_origH;
            if (s_dragHandle == 0 || s_dragHandle == 6 || s_dragHandle == 7) { nw = s_origW - (int)d.x; nx = s_origX + (int)d.x; }
            if (s_dragHandle == 0 || s_dragHandle == 1 || s_dragHandle == 2) { nh = s_origH - (int)d.y; ny = s_origY + (int)d.y; }
            if (s_dragHandle == 2 || s_dragHandle == 3 || s_dragHandle == 4) { nw = s_origW + (int)d.x; }
            if (s_dragHandle == 4 || s_dragHandle == 5 || s_dragHandle == 6) { nh = s_origH + (int)d.y; }
            nw = ImMax(nw, 12); nh = ImMax(nh, 8);
            if (g_snapEnabled) {
                nx = (nx + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE;
                ny = (ny + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE;
                nw = (nw + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE;
                nh = (nh + GRID_SIZE/2) / GRID_SIZE * GRID_SIZE;
            }
            s_sel->x = nx; s_sel->y = ny; s_sel->w = nw; s_sel->h = nh;
            g_modelDirty = true;
        }
    }

    // ---- yerleştirme modu ----
    if (!g_armTool.empty()) {
        dl->AddRect(rectMin, rectMax, IM_COL32(90, 200, 120, 200), 4.0f, 0, 2.0f);
        if (ImGui::IsMouseClicked(0) && !io.WantCaptureKeyboard) {
            ImVec2 pt = io.MousePos;
            if (pt.x > rectMin.x && pt.x < rectMax.x && pt.y > rectMin.y && pt.y < rectMax.y) {
                int px = (int)(pt.x - formPos.x), py = (int)(pt.y - formPos.y);
                if (g_snapEnabled) { px = px / GRID_SIZE * GRID_SIZE; py = py / GRID_SIZE * GRID_SIZE; }
                // tıklanan nokta bir konteynerin içinde mi? (basit: çizilen hitRects'ten üstteki konteyner)
                Ctrl* parent = nullptr;
                for (int i = (int)s_hitRects.size() - 1; i >= 0; i--) {
                    Ctrl* cc = s_hitRects[i].c;
                    if (IsContainerType(cc->type) && !cc->nonVisual) {
                        WidgetRect& r = s_hitRects[i];
                        if (pt.x >= r.rmin.x && pt.x <= r.rmax.x && pt.y >= r.rmin.y && pt.y <= r.rmax.y) {
                            parent = cc;
                            px = (int)(pt.x - r.rmin.x); py = (int)(pt.y - r.rmin.y);
                            break;
                        }
                    }
                }
                Ctrl* added = g_model.Add(g_armTool, px, py, parent);
                if (added) {
                    // konteyner çocuğu olarak parent'a eklendi — parent'ın çocuk listesine push edildi (model.Add parent'ı set eder)
                }
                g_armTool.clear();
                g_armParent = nullptr;
            } else {
                g_armTool.clear();   // canvas dışı tık = iptal
            }
        }
    }

    // seçim tıklaması (drag değilken + yerleştirme yokken) — canvas boş alanı
    if (!s_dragBody && s_dragHandle < 0 && g_armTool.empty() && ImGui::IsMouseClicked(0) &&
        io.MousePos.x > rectMin.x && io.MousePos.x < rectMax.x && io.MousePos.y > rectMin.y && io.MousePos.y < rectMax.y) {
        // ImGui'nin kendi penceresi bu tklığı yedi mi kontrol et — GetItemRect'lerle değil: IsAnyItemHovered
        if (!ImGui::IsAnyItemHovered()) {
            // canvas arka planına tıklandı → form seçimi (ya da alttaki nesne)
            ClickSelectUnder(io.MousePos);
        } else {
            // nesne üstünde tık → seç (nesne kendisi toggle olabilir — kabul)
            ClickSelectUnder(io.MousePos);
        }
    }

    // durum çubuğu ipucu
    static char s_canvasInfo[128];
    snprintf(s_canvasInfo, sizeof s_canvasInfo, "Canvas: %s | %d nesne", (g_armTool.empty() ? "se\303\247im" : "yerle\305\237tir: " + g_armTool).c_str(), (int)m.ctrls.size());
}

} // namespace canvas
