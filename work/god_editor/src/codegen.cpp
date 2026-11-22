// ============================================================================
//  codegen.cpp — ImGui kod üreticisi: editör tasarımı → tam VS2019/MinGW projesi
//  Üretilen uygulama derlenince editördeki form AYNEN ortaya çıkar (WYSIWYG:
//  canvas ve üretim aynı ImGui çağrı dizisini kullanır).
// ============================================================================
#include "codegen.h"
#include <cstring>
#include "objstyle.h"
#include "forms.h"
#include "theme.h"
#include "imgod.h"
#include <sstream>
#include <set>

namespace codegen {

GenSettings g_settings;

std::string HandlerOf(const std::string& obj, const std::string& ev) {
    std::string h = obj;
    for (auto& c : h) if (c == ' ' || c == '-' || c == '.') c = '_';
    return h + "_" + ev;
}

// ---------------------------------------------------------------------------
//  yardımcılar
static std::string NumOf(const Ctrl* c, const char* k, const char* def) {
    auto it = c->props.find(k);
    if (it == c->props.end()) return def;
    return it->second.empty() ? std::string(def) : TrimA(it->second);
}
static std::string StrOf(const Ctrl* c, const char* k, const char* def) {
    auto it = c->props.find(k);
    return it == c->props.end() ? std::string(def) : it->second;
}
static std::string FNum(double d) {
    char b[64]; snprintf(b, sizeof b, "%.6g", d);
    std::string s = b;
    // C++ standardı: 40f GEÇERSİZ — kayan nokta sabiti '.' ya da üs İSTER.
    // Bu yüzden daima ondalık nokta garanti et: 40 → 40.0f
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find("inf") == std::string::npos && s.find("nan") == std::string::npos)
        s += ".0";
    if (s.find("inf") == std::string::npos && s.find("nan") == std::string::npos) s += "f";
    return s;
}
static std::string FNumS(const std::string& v, double def) {
    if (v.empty()) return FNum(def);
    char* e = nullptr; double d = strtod(v.c_str(), &e);
    if (e == v.c_str()) return FNum(def);
    return FNum(d);
}
static std::string INum(const std::string& v, long def) {
    if (v.empty()) return std::to_string(def);
    char* e = nullptr; long d = strtol(v.c_str(), &e, 10);
    if (e == v.c_str()) return std::to_string(def);
    return std::to_string(d);
}
static std::string Vec2Arg(int w, int h) {
    if (w > 0 || h > 0) return "ImVec2(" + std::to_string(w) + ", " + std::to_string(h) + ")";
    return "ImVec2(0, 0)";
}
static std::string FormFlagsOf(const Model& m) {
    // formProps["Flags"]: "NoTitleBar|NoResize|NoCollapse|AlwaysAutoResize" (| ile)
    std::string f = "0";
    auto it = m.formProps.find("Flags");
    if (it == m.formProps.end()) return f;
    for (auto& s : SplitW(Utf8ToWideStr(it->second), L'|')) {
        std::string t = TrimA(WideStrToUtf8(s));
        if (t == "NoTitleBar") f += " | ImGuiWindowFlags_NoTitleBar";
        else if (t == "NoResize") f += " | ImGuiWindowFlags_NoResize";
        else if (t == "NoCollapse") f += " | ImGuiWindowFlags_NoCollapse";
        else if (t == "NoMove") f += " | ImGuiWindowFlags_NoMove";
        else if (t == "AlwaysAutoResize") f += " | ImGuiWindowFlags_AlwaysAutoResize";
        else if (t == "NoScrollbar") f += " | ImGuiWindowFlags_NoScrollbar";
        else if (t == "MenuBar") f += " | ImGuiWindowFlags_MenuBar";
    }
    return f;
}

// ---------------------------------------------------------------------------
//  nesne → C++ satır(lar)ı üretimi (canvas ile BİREBİR aynı çağrı dizisi)
struct EmitCtx {
    std::ostringstream body;      // Render() gövdesi
    std::ostringstream decls;     // sınıf içindeki nesne bildirimleri
    std::ostringstream handlers; // handler prototipleri + boş gövdeler
    std::ostringstream initlist; // yapıcı içindeki ek bağlamalar
    const Model* m = nullptr;     // çocuk taraması için emit edilen model
};

static void EmitEventCall(EmitCtx& ctx, const Ctrl* c, const std::string& cond, const std::string& ev) {
    auto it = c->events.find(ev);
    std::string h = it != c->events.end() && !it->second.empty() ? it->second : HandlerOf(c->name, ev);
    ctx.body << "    if (" << cond << ") " << h << "();\n";
}

// tasarım özelliği → initlist bağlama satırı (WinForms InitializeComponent ruhu:
// değerler FORM YAPICISINDA bir kez bağlanır, kullanıcı kodu runtime'da serbestçe değiştirir)
static void Bind(EmitCtx& ctx, const Ctrl* c, const std::string& line) {
    ctx.initlist << "    " << c->name << "." << line << ";\n";
}
// "Seçenek##checkbox1" biçimli benzersiz etiket (görünen metin + ID soneki)
static std::string ULabel(const Ctrl* c, const char* textProp, const char* defText) {
    return "u8\"" + EscCppUtf8(StrOf(c, textProp, defText)) + "##" + c->name + "\"";
}
static std::string UPlain(const std::string& s) {   // düz u8 sabiti
    return "u8\"" + EscCppUtf8(s) + "\"";
}
static std::string FVec2(int w, int h) {
    return "ImVec2(" + std::to_string(w) + ", " + std::to_string(h) + ")";
}
static std::string FVec4(float r, float g, float b, float a) {
    return "ImVec4(" + FNum(r) + "f, " + FNum(g) + "f, " + FNum(b) + "f, " + FNum(a) + "f)";
}

static void EmitCtrl(EmitCtx& ctx, const Ctrl* c) {
    const Model& m = *ctx.m;
    const std::string& t = c->type;
    const std::string pos = "ImVec2(" + std::to_string(c->x) + ", " + std::to_string(c->y) + ")";
    // stil/renk geçersiz kılmaları — canvas ile AYNI push dizisi (WYSIWYG)
    const std::string stPush = objstyle::EmitPush(c);
    const std::string stPop  = objstyle::EmitPop(objstyle::CountPushes(c));
    const bool disabled = c->GetB("Disabled");
    if (!stPush.empty()) ctx.body << stPush;
    if (disabled) ctx.body << "    ImGui::BeginDisabled();\n";

    if (t == "Button") {
        int w = (int)strtol(NumOf(c, "SizeArgW", "120").c_str(), nullptr, 10);
        int h = (int)strtol(NumOf(c, "SizeArgH", "28").c_str(), nullptr, 10);
        std::string size = (c->w > 0 || c->h > 0) ? Vec2Arg(c->w, c->h) : Vec2Arg(w, h);
        if (c->GetB("Repeat")) ctx.body << "    ImGui::PushButtonRepeat(true);\n";
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        if (c->GetB("Small"))
            ctx.body << "    if (ImGui::SmallButton(" << ULabel(c, "Text", "Buton") << ")) " << HandlerOf(c->name, "Click") << "();\n";
        else
            ctx.body << "    if (ImGui::Button(" << ULabel(c, "Text", "Buton") << ", " << size << ")) " << HandlerOf(c->name, "Click") << "();\n";
        if (c->GetB("Repeat")) ctx.body << "    ImGui::PopButtonRepeat();\n";
        auto it = c->events.find("Hovered");
        if (it != c->events.end() && !it->second.empty())
            ctx.body << "    if (ImGui::IsItemHovered()) " << it->second << "();\n";
        if (!StrOf(c, "Tooltip", "").empty())
            ctx.body << "    if (ImGui::IsItemHovered()) ImGui::SetTooltip(" << UPlain(StrOf(c, "Tooltip", "")) << ");\n";
        ctx.decls << "    // " << c->name << " : Button — tasarım: " << StrOf(c, "Text", "") << "\n";
    }
    else if (t == "Checkbox") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Seçenek"));
        // BOYUT (WYSIWYG): genişlik/yükseklik — canvas ile aynı dönüşüm
        // (FramePadding ölçekleme + etiket kısaltma) IGOD::Checkbox içinde.
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        Bind(ctx, c, "Height = " + std::to_string(c->h > 0 ? c->h : 0));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Checked0 = " << (c->GetB("Checked") ? "true" : "false") << ";\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "Checked") << "();\n";
        ctx.decls << "    IGOD::Checkbox " << c->name << ";   // tasarım: " << StrOf(c, "Text", "") << "\n";
    }
    else if (t == "RadioButton") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Seçim"));
        Bind(ctx, c, std::string("Active = ") + (c->GetB("Active") ? "true" : "false"));
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        Bind(ctx, c, "Height = " + std::to_string(c->h > 0 ? c->h : 0));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "Click") << "();\n";
        ctx.decls << "    IGOD::RadioButton " << c->name << ";\n";
    }
    else if (t == "SliderFloat" || t == "SliderInt" || t == "DragFloat" || t == "DragInt" ||
             t == "InputText" || t == "InputTextMultiline" || t == "TextBox" || t == "TextBoxMultiline" ||
             t == "InputInt" || t == "InputFloat") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "InputText" || t == "InputTextMultiline" || t == "TextBox" || t == "TextBoxMultiline" ? "Yazı" : "Değer"));
        if (t == "SliderFloat" || t == "DragFloat" || t == "InputFloat") {
            Bind(ctx, c, "Value = " + FNum((float)atof(NumOf(c, "Value", "0").c_str())));
            Bind(ctx, c, "Min = " + FNum((float)atof(NumOf(c, "Min", "0").c_str())));
            Bind(ctx, c, "Max = " + FNum((float)atof(NumOf(c, "Max", "100").c_str())));
        } else if (t == "SliderInt" || t == "DragInt" || t == "InputInt") {
            Bind(ctx, c, "Value = " + std::to_string((int)strtol(NumOf(c, "Value", "0").c_str(), nullptr, 10)));
            Bind(ctx, c, "Min = " + std::to_string((int)strtol(NumOf(c, "Min", "0").c_str(), nullptr, 10)));
            Bind(ctx, c, "Max = " + std::to_string((int)strtol(NumOf(c, "Max", "100").c_str(), nullptr, 10)));
        }
        if (t == "SliderFloat" || t == "SliderInt") Bind(ctx, c, "Format = " + UPlain(StrOf(c, "Format", t == "SliderFloat" ? "%.1f" : "%d")));
        if (t == "DragFloat" || t == "DragInt")    Bind(ctx, c, "Speed = " + FNum((float)atof(NumOf(c, "Speed", "1").c_str())));
        if (t == "InputInt" || t == "InputFloat")   Bind(ctx, c, "Step = " + FNum((float)atof(NumOf(c, "Step", "1").c_str())));
        if (t == "InputText" || t == "TextBox") {
            Bind(ctx, c, "Value = " + UPlain(StrOf(c, "Value", "")));
            Bind(ctx, c, "Hint = " + UPlain(StrOf(c, "Hint", "")));
            Bind(ctx, c, std::string("Password = ") + (c->GetB("Password") ? "true" : "false"));
            Bind(ctx, c, std::string("ReadOnly = ") + (c->GetB("ReadOnly") ? "true" : "false"));
        }
        if (t == "InputTextMultiline" || t == "TextBoxMultiline") {
            Bind(ctx, c, "Value = " + UPlain(StrOf(c, "Value", "")));
            Bind(ctx, c, "ReadOnly = " + std::string(c->GetB("ReadOnly") ? "true" : "false"));
            Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "280").c_str(), nullptr, 10),
                                            c->h ? c->h : (int)strtol(NumOf(c, "Height", "120").c_str(), nullptr, 10)));
        }
        // BOYUT (WYSIWYG): genişlik tasarım değerinden — canvas SetNextItemWidth ile
        // AYNI değeri uygular; derlenen uygulama da birebir aynı genişlikte çizilir.
        // (Multiline hariç — onun Size(w,h) zaten bağlı)
        if (t != "InputTextMultiline" && t != "TextBoxMultiline")
            Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        std::string defEv = (t == "InputText" || t == "InputTextMultiline" || t == "TextBox" || t == "TextBoxMultiline") ? "TextChanged" : "ValueChanged";
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, defEv) << "();\n";
        // TextBox = ImGui::InputText sarmalayıcısı (IGOD::InputText) — ad M2God sözlüğü
        ctx.decls << "    IGOD::" << (t == "TextBox" ? "InputText" : (t == "TextBoxMultiline" ? "InputTextMultiline" : t)) << " " << c->name << ";   // tasarım: " << StrOf(c, "Text", "") << "\n";
        if (t == "InputText" || t == "TextBox") {
            auto it = c->events.find("EnterPressed");
            if (it != c->events.end() && !it->second.empty())
                ctx.body << "    if (" << c->name << ".EnterPressed) " << it->second << "();\n";
        }
    }
    else if (t == "Combo" || t == "ListBox") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "Combo" ? "Seçim" : "Liste"));
        std::string items;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Items", "")), L'|')) {
            if (!items.empty()) items += ", ";
            items += UPlain(WideStrToUtf8(s));
        }
        Bind(ctx, c, "Items = { " + items + " }");
        Bind(ctx, c, "Selected = " + std::to_string((int)strtol(NumOf(c, "Selected", "0").c_str(), nullptr, 10)));
        if (t == "ListBox")
            Bind(ctx, c, "HeightItems = " + std::to_string((int)strtol(NumOf(c, "HeightItems", "5").c_str(), nullptr, 10)));
        // BOYUT (WYSIWYG): Combo genişliği SetNextItemWidth ile; ListBox TAM boyutu
        // (w,h) BeginListBox'a — canvas'la birebir aynı değerler.
        if (t == "Combo")
            Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        if (t == "ListBox")
            Bind(ctx, c, "Size = " + Vec2Arg(c->w, c->h));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "SelectionChanged") << "();\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";\n";
    }
    else if (t == "ColorEdit4") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Renk"));
        Bind(ctx, c, "Col = " + FVec4((float)atof(NumOf(c, "R", "255").c_str()) / 255.0f,
                                       (float)atof(NumOf(c, "G", "255").c_str()) / 255.0f,
                                       (float)atof(NumOf(c, "B", "255").c_str()) / 255.0f,
                                       (float)atof(NumOf(c, "A", "255").c_str()) / 255.0f));
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));   // WYSIWYG genişlik
        Bind(ctx, c, std::string("NoInputs = ") + (c->GetB("AlphaPreview") ? "false" : "true"));   // canvas kipiyle aynı
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "ColorChanged") << "();\n";
        ctx.decls << "    IGOD::ColorEdit4 " << c->name << ";\n";
    }
    else if (t == "ProgressBar") {
        Bind(ctx, c, "Value = " + FNum((float)atof(NumOf(c, "Value", "0.5").c_str())));
        Bind(ctx, c, "Overlay = " + UPlain(StrOf(c, "Overlay", "")));
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "200").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "18").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::ProgressBar " << c->name << ";\n";
    }
    else if (t == "Separator") {
        // BOYUT UYGULA (W+H): gerçek ayraç — W×H dolgu çubuğu (IGOD::Separator).
        // Canvas AYNI çubuğu AddRectFilled ile çizer (WYSIWYG).
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "200").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "2").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::Separator " << c->name << ";\n";
    }
    else if (t == "SeparatorText") {
        // BOYUT UYGULA (W): çizgi w'ye kadar — IGOD::SeparatorText child-clip sarması.
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Bölüm"));
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::SeparatorText " << c->name << ";\n";
    }
    else if (t == "Text" || t == "TextColored" || t == "BulletText" || t == "LabelText") {
        // kimliksiz çıktılar — ## soneki GÖRÜNÜR, bu yüzden düz metin (çakışma yok)
        std::string s = EscCppUtf8(StrOf(c, "Text", "Etiket"));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        if (t == "Text") {
            // BOYUT UYGULA (W): PushTextWrapPos — metin w'de kayar (canvas aynı).
            if (c->w > 0) {
                ctx.body << "    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + " << FNum((double)c->w) << ");\n";
            }
            if (c->GetB("Wrapped") && c->w <= 0) ctx.body << "    ImGui::TextWrapped(" << UPlain(StrOf(c, "Text", "Etiket")) << ");\n";
            else ctx.body << "    ImGui::Text(" << UPlain(StrOf(c, "Text", "Etiket")) << ");\n";
            if (c->w > 0) ctx.body << "    ImGui::PopTextWrapPos();\n";
        }
        else if (t == "TextColored") {
            int r = (int)strtol(NumOf(c, "R", "255").c_str(), nullptr, 10);
            int g = (int)strtol(NumOf(c, "G", "200").c_str(), nullptr, 10);
            int b = (int)strtol(NumOf(c, "B", "80").c_str(), nullptr, 10);
            if (c->w > 0) {
                ctx.body << "    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + " << FNum((double)c->w) << ");\n";
            }
            ctx.body << "    ImGui::TextColored(" << FVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f) << ", " << UPlain(StrOf(c, "Text", "Renkli etiket")) << ");\n";
            if (c->w > 0) ctx.body << "    ImGui::PopTextWrapPos();\n";
        }
        else if (t == "BulletText") {
            // BOYUT UYGULA (W): kaydırma yok → etiket w'ye sığdırılır (IGOD aynı).
            Bind(ctx, c, "Text = " + UPlain(StrOf(c, "Text", "Madde")));
            Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
            ctx.body << "    " << c->name << ".Draw();\n";
            ctx.decls << "    IGOD::BulletText " << c->name << ";\n";
        }
        else {
            // LabelText — BOYUT UYGULA (W): değer öğe genişliğinin sağ ucunda
            // hizalanır (CalcItemWidth). Canvas aynı SetNextItemWidth'ı uygular.
            if (c->w > 0)
                ctx.body << "    ImGui::SetNextItemWidth(" << FNum((double)c->w) << ");\n";
            ctx.body << "    ImGui::LabelText(" << ULabel(c, "Text", "Alan") << ", " << UPlain(StrOf(c, "Value", "değer")) << ");\n";
        }
    }
    else if (t == "PlotLines" || t == "PlotHistogram") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "PlotLines" ? "Grafik" : "Histogram"));
        std::string data;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Items", "")), L'|')) {
            if (!data.empty()) data += ", ";
            data += FNum((float)wcstod(s.c_str(), nullptr));
        }
        if (data.empty()) data = "0.0f";
        Bind(ctx, c, "Data = { " + data + " }");
        if (t == "PlotLines") {
            Bind(ctx, c, "ScaleMin = " + FNum((float)atof(NumOf(c, "ScaleMin", "0").c_str())));
            Bind(ctx, c, "ScaleMax = " + FNum((float)atof(NumOf(c, "ScaleMax", "1").c_str())));
        }
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "280").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "120").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";\n";
    }
    else if (t == "Panel") {
        // PANEL — WinForms Panel: kenarlıklı child bölge (ImGui::BeginChild).
        // Nesneler İÇİNE yerleştirilebilir; editör canvas'ı aynı çizimi kullanır.
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Panel"));   // ## sonekli benzersiz child kimliği
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "300").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "200").c_str(), nullptr, 10)));
        Bind(ctx, c, std::string("Border = ") + (c->GetB("Border") ? "true" : "false"));
        Bind(ctx, c, "Rounding = " + FNumS(NumOf(c, "Rounding", "-1"), -1.0));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Begin();\n";
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) EmitCtrl(ctx, ch);
        ctx.body << "    " << c->name << ".End();\n";
        ctx.decls << "    IGOD::Panel " << c->name << ";\n";
    }
    else if (t == "ChildWindow") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Child"));   // ## sonekli: child kimliği benzersiz
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "300").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "200").c_str(), nullptr, 10)));
        Bind(ctx, c, std::string("Border = ") + (c->GetB("Border") ? "true" : "false"));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Begin();\n";
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) EmitCtrl(ctx, ch);
        ctx.body << "    " << c->name << ".End();\n";
        ctx.decls << "    IGOD::ChildWindow " << c->name << ";\n";
    }
    else if (t == "GroupBox") {
        Bind(ctx, c, "Text = " + UPlain(StrOf(c, "Text", "Grup")));   // görünür başlık
        Bind(ctx, c, "Id = " + UPlain(c->name));                        // benzersiz child kimliği
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "320").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "160").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Begin();\n";
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) EmitCtrl(ctx, ch);
        ctx.body << "    " << c->name << ".End();\n";
        ctx.decls << "    IGOD::GroupBox " << c->name << ";\n";
    }
    else if (t == "CollapsingHeader" || t == "TreeNode") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "CollapsingHeader" ? "Başlık" : "Düğüm"));
        Bind(ctx, c, std::string("DefaultOpen = ") + (c->GetB("DefaultOpen") ? "true" : "false"));
        // BOYUT UYGULA (W): başlık w'lik child bölgeye sınırlanır (IGOD aynı).
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Begin()) {\n";
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) EmitCtrl(ctx, ch);
        ctx.body << "    }\n";
        ctx.body << "    " << c->name << ".End();   // child dengesi — her zaman\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";\n";
    }
    else if (t == "TabBar") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Sekmeler"));   // ## sonekli tabbar kimliği
        // BOYUT UYGULA (W+H): sekmeler W×H child bölgesine çizilir (canvas aynı).
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "400").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "220").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Begin()) {\n";
        std::string items = StrOf(c, "Items", "Genel|Ayarlar");
        for (auto& s : SplitW(Utf8ToWideStr(items), L'|')) {
            std::string title = WideStrToUtf8(s);
            ctx.body << "    if (ImGui::BeginTabItem(" << UPlain(title) << ")) {\n";
            for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) {
                std::string owner = ch->Get("TabOwner", "");
                if (owner == title) EmitCtrl(ctx, ch);
            }
            ctx.body << "        ImGui::EndTabItem();\n    }\n";
        }
        ctx.body << "    }\n";
        ctx.body << "    " << c->name << ".End();   // child dengesi — her zaman\n";
        ctx.decls << "    IGOD::TabBar " << c->name << ";\n";
    }
    else if (t == "Popup") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Pencere"));    // modal başlığı + benzersiz kimlik
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::ModalPopup " << c->name << ";\n";
    }
    else if (t == "Table") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Tablo"));      // ## sonekli tablo kimliği
        Bind(ctx, c, "Columns = " + std::to_string((int)strtol(NumOf(c, "Columns", "3").c_str(), nullptr, 10)));
        Bind(ctx, c, "Rows = " + std::to_string((int)strtol(NumOf(c, "Rows", "3").c_str(), nullptr, 10)));
        std::string heads;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Items", "Sütun1|Sütun2|Sütun3")), L'|')) {
            if (!heads.empty()) heads += ", ";
            heads += UPlain(WideStrToUtf8(s));
        }
        Bind(ctx, c, "Headers = { " + heads + " }");
        Bind(ctx, c, "Width = " + std::to_string(c->w > 0 ? c->w : 0));   // WYSIWYG genişlik
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::Table " << c->name << ";\n";
    }
    else if (t == "Menu") {
        std::string menus;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Items", "Dosya|Düzen|Yardım")), L'|')) {
            if (!menus.empty()) menus += ", ";
            menus += UPlain(WideStrToUtf8(s));
        }
        std::string cmds;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Commands", "")), L'|')) {
            if (cmds.empty()) continue;
            if (!cmds.empty()) cmds += ", ";
            cmds += UPlain(WideStrToUtf8(s));
        }
        Bind(ctx, c, "Menus = { " + menus + " }");
        Bind(ctx, c, "Commands = { " + cmds + " }");
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "MenuClick") << "();\n";
        ctx.decls << "    IGOD::MenuBarWrap " << c->name << ";\n";
    }
    else if (t == "Timer") {
        ctx.decls << "    IGOD::Timer " << c->name << ";   // Interval=" << StrOf(c, "Interval", "1000") << "ms\n";
        ctx.initlist << "    " << c->name << ".Interval = " << INum(StrOf(c, "Interval", "1000"), 1000) << ";\n";
        ctx.initlist << "    " << c->name << ".Enabled = " << (c->GetB("Enabled") ? "true" : "false") << ";\n";
        ctx.initlist << "    " << c->name << ".Tick = []{ " << HandlerOf(c->name, "Tick") << "(); };\n";
        ctx.initlist << "    Timers.push_back(&" << c->name << ");\n";
    }
    else if (t == "FontRef") {
        Bind(ctx, c, "Name = " + UPlain(StrOf(c, "Text", "Arial")));
        Bind(ctx, c, "Size = " + FNum((float)atof(NumOf(c, "Size", "18").c_str())));
        Bind(ctx, c, std::string("Bold = ") + (c->GetB("Bold") ? "true" : "false"));
        ctx.decls << "    IGOD::FontRef " << c->name << ";\n";
    }
    else {
        ctx.body << "    /* bilinmeyen nesne: " << t << " " << c->name << " */\n";
    }

    if (disabled) ctx.body << "    ImGui::EndDisabled();\n";
    if (!stPop.empty()) ctx.body << stPop;
}

// ---------------------------------------------------------------------------
//  Form.cpp / Form.h üretimi
std::string EmitFormCpp(const std::string& formName, const Model& m) {
    EmitCtx ctx;
    ctx.m = &m;
    std::string title = m.formProps.count("Text") ? m.formProps.at("Text") : formName;
    std::string flags = FormFlagsOf(m);

    std::ostringstream o;
    o << "// " << formName << ".cpp — IMGUI_GOD_SSSTIER editöründen ÜRETİLDİ\n";
    o << "// Değişiklikleri USER bölgelerine yazın; editör üretim bloklarını yeniler.\n";
    o << "#include \"" << formName << ".h\"\n\n";
    o << "//== IMGOD:INIT-BEGIN (editör üretimi — elle değiştirmeyin)\n";
    o << "void " << formName << "::Render() {\n";
    // ---- form stili (varsa) — canvas ile AYNI push dizisi (WYSIWYG) ----
    int _nStyleCol = 0, _nStyleVar = 0;
    {
        float f[4];
        if (objstyle::ParseHexColor(m.formProps.count("WindowBgColor") ? m.formProps.at("WindowBgColor") : "", f)) {
            o << "    ImGui::PushStyleColor(ImGuiCol_WindowBg, " << FVec4(f[0], f[1], f[2], f[3]) << ");\n";
            _nStyleCol++;
        }
        if (m.formProps.count("WindowRounding") && (float)atof(m.formProps.at("WindowRounding").c_str()) >= 0) {
            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, " << FNum((float)atof(m.formProps.at("WindowRounding").c_str())) << "f);\n";
            _nStyleVar++;
        }
        if (m.formProps.count("WindowBorderSize") && (float)atof(m.formProps.at("WindowBorderSize").c_str()) >= 0) {
            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, " << FNum((float)atof(m.formProps.at("WindowBorderSize").c_str())) << "f);\n";
            _nStyleVar++;
        }
    }
    o << "    ImGui::SetNextWindowSize(ImVec2(" << m.formW << ", " << m.formH << "), ImGuiCond_FirstUseEver);\n";
    o << "    if (!ImGui::Begin(u8\"" << EscCppUtf8(title) << "\", nullptr, " << flags << ")) { ImGui::End();\n";
    if (_nStyleVar > 0) o << "        ImGui::PopStyleVar(" << _nStyleVar << ");\n";
    if (_nStyleCol > 0) o << "        ImGui::PopStyleColor(" << _nStyleCol << ");\n";
    o << "        return; }\n";
    // üst-seviye nesneler (parent=null) — üstte kalan son
    std::vector<Ctrl*> tops;
    for (Ctrl* c : m.ctrls) if (!c->parent) tops.push_back(c);
    for (Ctrl* c : tops) EmitCtrl(ctx, c);
    o << ctx.body.str();
    o << "    ImGui::End();\n";
    if (_nStyleVar > 0) o << "    ImGui::PopStyleVar(" << _nStyleVar << ");\n";
    if (_nStyleCol > 0) o << "    ImGui::PopStyleColor(" << _nStyleCol << ");\n";
    o << "}\n";
    o << "//== IMGOD:INIT-END\n\n";
    // olay işleyicileri
    o << "//== IMGOD:HANDLERS-BEGIN\n";
    std::set<std::string> emitted;
    for (Ctrl* c : m.ctrls) {
        const ElementDef* def = g_catalog.Find(c->type);
        if (!def) continue;
        for (auto& ev : def->events) {
            auto it = c->events.find(ev.name);
            std::string h = it != c->events.end() && !it->second.empty() ? it->second : HandlerOf(c->name, ev.name);
            if (!emitted.insert(h).second) continue;
            o << "void " << formName << "::" << h << "() {\n";
            o << "    // TODO: " << c->name << " → " << ev.name << " (" << ev.desc << ") — kodunuzu yazın\n";
            o << "}\n\n";
        }
    }
    // form olayları (Load)
    for (auto& kv : m.formEvents) {
        std::string h = kv.second.empty() ? HandlerOf(formName, kv.first) : kv.second;
        if (!emitted.insert(h).second) continue;
        o << "void " << formName << "::" << h << "() {\n    // TODO: form " << kv.first << " olayı\n}\n\n";
    }
    o << "//== IMGOD:HANDLERS-END\n\n";
    // yapıcı: timer kayıtları + child içerik senkronları
    o << formName << "::" << formName << "() {\n";
    o << ctx.initlist.str();
    o << "}\n";
    o << formName << "::~" << formName << "() {}\n";
    o << "void " << formName << "::OnLoad() {\n    // TODO: form yüklendiğinde çalışacak kod\n}\n";
    return o.str();
}

std::string EmitFormH(const std::string& formName, const Model& m) {
    EmitCtx ctx;
    ctx.m = &m;
    // decls için tipler (render üretmeden decl toplamak için)
    std::ostringstream o;
    o << "// " << formName << ".h — IMGUI_GOD_SSSTIER editöründen ÜRETİLDİ\n";
    o << "#pragma once\n#include \"Global.h\"\n\n";
    o << "struct " << formName << " : IGOD::FormBase {\n";
    // nesneler — decl string'ini topla (render emit etmeden sadece decls doldurmak için
    // ayrı bir ctx ile tarama)
    EmitCtx dctx;
    dctx.m = &m;   // KRİTİK: model bağlanmadan konteyner çocuk taraması null modelle çökerdi
    // HATA DÜZELTİLDİ (5. tur): eski kod TÜM ctrls'i dolaşıyordu — konteyner
    // EmitCtrl'i çocuklarını da özyinelemeli eklediğinden çocuklar İKİ KEZ
    // decl edilip Form1.h'te çift bildirim (derleme hatası) üretiyordu.
    // Button decl'i yorum olduğundan eski testler bunu yakalayamadı; TextBox
    // gerçek bildirim üretince ortaya çıktı. Artık yalnız KÖK nesneler
    // dolaşılır — konteyner özyinelemesi çocukları zaten ekler.
    for (Ctrl* c : m.ctrls) if (!c->parent) EmitCtrl(dctx, c);
    o << dctx.decls.str();
    // handler prototipleri
    std::set<std::string> emitted;
    for (Ctrl* c : m.ctrls) {
        const ElementDef* def = g_catalog.Find(c->type);
        if (!def) continue;
        for (auto& ev : def->events) {
            auto it = c->events.find(ev.name);
            std::string h = it != c->events.end() && !it->second.empty() ? it->second : HandlerOf(c->name, ev.name);
            if (emitted.insert(h).second) o << "    void " << h << "();   // " << c->name << ": " << ev.desc << "\n";
        }
    }
    for (auto& kv : m.formEvents) {
        std::string h = kv.second.empty() ? HandlerOf(formName, kv.first) : kv.second;
        if (emitted.insert(h).second) o << "    void " << h << "();   // form olayı: " << kv.first << "\n";
    }
    o << "    void OnLoad() override;\n";
    o << "    " << formName << "();\n";
    o << "    ~" << formName << "() override;   // vtable bu TU'da bağlanır\n";
    o << "    void Render() override;\n";
    o << "};\n";
    return o.str();
}

// ---------------------------------------------------------------------------
//  Global.h — IGOD SDK (M2God sarmalayıcı ruhu, ImGui evreni)
std::string EmitGlobalH() {
    return R"IGODSDK(// ============================================================================
// Global.h — IGOD SDK v1 (IMGUI GOD SSS TIER)
// WinForms benzeri sözdizimiyle ImGui nesneleri: button1->Text, slider1->Value
// Tema paletleri (GOD OPS / Midnight Steel / Ammo Tan), Timer, çok formlu runtime.
// ImGui >= 1.90 ile derlenir (ürün 1.91.8 ile test edildi).
// ============================================================================
#pragma once
#include "imgui.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace IGOD {

// —— ortak yardımcı ———————————————————————————————————————————————————
// etiketi avail genişliğine sığdır (taşarsa "..." ile kısalt, UTF-8 güvenli).
// Editör canvas'ı AYNI algoritmayı kullanır (WYSIWYG).
inline std::string FitTextToWidth(const std::string& text, float avail) {
    if (avail <= 8.0f) return text;
    if (ImGui::CalcTextSize(text.c_str(), NULL, true).x <= avail) return text;
    const std::string ell = "...";
    float budget = avail - ImGui::CalcTextSize(ell.c_str()).x;
    size_t i = 0;
    while (i < text.size()) {
        size_t len = 1;
        unsigned char ch = (unsigned char)text[i];
        if (ch >= 0xF0) len = 4; else if (ch >= 0xE0) len = 3; else if (ch >= 0xC0) len = 2;
        if (i + len > text.size()) len = text.size() - i;
        if (ImGui::CalcTextSize(text.c_str(), text.c_str() + i + len, true).x > budget) break;
        i += len;
    }
    std::string out = text.substr(0, i);
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out + ell;
}

// —— nesne sarmalayıcıları ---------------------------------------------------
struct Button {
    std::string Text;  ImVec2 SizeArg = ImVec2(0, 0);
    bool Hovered = false, Held = false, Clicked = false;
    Button() {}
    Button(const std::string& t, ImVec2 sz) : Text(t), SizeArg(sz) {}
    bool Draw() {
        Clicked = ImGui::Button(Text.c_str(), SizeArg);
        Hovered = ImGui::IsItemHovered();
        Held = ImGui::IsItemActive();
        return Clicked;
    }
    bool Draw(const ImVec2& at) {
        ImGui::SetCursorPos(at);
        return Draw();
    }
};
struct Checkbox {
    std::string Text;  bool Checked = false;
    bool Checked0 = false;         // tasarım başlangıcı (her karede senkron — editör WYSIWYG)
    float Width = 0, Height = 0;   // tasarım boyutu (WYSIWYG — canvas aynı dönüşümü uygular)
    Checkbox() {}
    Checkbox(const std::string& t, bool v) : Text(t), Checked(v), Checked0(v) {}
    bool Draw() {
        if (Checked0) { Checked = true; Checked0 = false; }
        bool prev = Checked;
        // BOYUT (WYSIWYG): yükseklik → FramePadding ölçekleme (kare + satır büyür);
        // genişlik → etiket sığdırma. Editör canvas'ı birebir aynı yapar.
        bool padPush = false;
        if (Height > 0) {
            float padY = (Height - ImGui::GetFontSize()) * 0.5f;
            if (padY >= 0.0f) {
                ImVec2 fp = ImGui::GetStyle().FramePadding;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fp.x, padY));
                padPush = true;
            }
        }
        float square = ImGui::GetFrameHeight();
        std::string shown = Text;
        if (Width > 0)
            shown = FitTextToWidth(Text, Width - square - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Checkbox(shown.c_str(), &Checked);
        if (padPush) ImGui::PopStyleVar();
        return Checked != prev;
    }
};
struct RadioButton {
    std::string Text;  bool Active = false;
    float Width = 0, Height = 0;   // tasarım boyutu (WYSIWYG)
    bool Draw() {
        bool padPush = false;
        if (Height > 0) {
            float padY = (Height - ImGui::GetFontSize()) * 0.5f;
            if (padY >= 0.0f) {
                ImVec2 fp = ImGui::GetStyle().FramePadding;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fp.x, padY));
                padPush = true;
            }
        }
        float square = ImGui::GetFrameHeight();
        std::string shown = Text;
        if (Width > 0)
            shown = FitTextToWidth(Text, Width - square - ImGui::GetStyle().FramePadding.x * 2.0f);
        bool clicked = ImGui::RadioButton(shown.c_str(), Active);
        if (padPush) ImGui::PopStyleVar();
        return clicked;
    }
};
struct SliderFloat {
    std::string Text = "Değer";  float Value = 0, Min = 0, Max = 100;
    std::string Format = "%.1f";  float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    SliderFloat() {}
    SliderFloat(const std::string& t, float v, float mn, float mx) : Text(t), Value(v), Min(mn), Max(mx) {}
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::SliderFloat(Text.c_str(), &Value, Min, Max, Format.c_str());
    }
};
struct SliderInt {
    std::string Text = "Değer";  int Value = 0, Min = 0, Max = 100;
    std::string Format = "%d";  float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::SliderInt(Text.c_str(), &Value, Min, Max, Format.c_str());
    }
};
struct DragFloat {
    std::string Text = "Değer";  float Value = 0, Min = 0, Max = 100, Speed = 1;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::DragFloat(Text.c_str(), &Value, Speed, Min, Max);
    }
};
struct DragInt {
    std::string Text = "Değer";  int Value = 0, Min = 0, Max = 100; float Speed = 1;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::DragInt(Text.c_str(), &Value, Speed, Min, Max);
    }
};
struct InputText {
    std::string Text = "Yazı";   std::string Value;  std::string Hint;
    bool Password = false;  bool ReadOnly = false;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    char Buf[512] = {};  bool InitBuf = false;  bool EnterPressed = false;
    InputText() {}
    bool Draw() {
        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }
        ImGuiInputTextFlags fl = 0;
        if (Password) fl |= ImGuiInputTextFlags_Password;
        if (ReadOnly) fl |= ImGuiInputTextFlags_ReadOnly;
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        bool ch = ImGui::InputText(Text.c_str(), Buf, sizeof Buf, fl);
        if (!Hint.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Hint.c_str());
        EnterPressed = ImGui::IsItemEdited() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsItemDeactivated());
        if (ch) Value = Buf;
        return ch;
    }
};
struct InputTextMultiline {
    std::string Text = "Not";   std::string Value;  bool ReadOnly = false;  ImVec2 Size = ImVec2(280, 120);
    char Buf[4096] = {};  bool InitBuf = false;
    bool Draw() {
        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }
        ImGuiInputTextFlags _fl = ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0;
        bool ch = ImGui::InputTextMultiline(Text.c_str(), Buf, sizeof Buf, Size, _fl);
        if (ch) Value = Buf;
        return ch;
    }
};
struct InputInt {
    std::string Text = "Sayı";  int Value = 0, Step = 1;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::InputInt(Text.c_str(), &Value, Step);
    }
};
struct InputFloat {
    std::string Text = "Sayı";  float Value = 0; float Step = 1;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG)
    bool Draw() {
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        return ImGui::InputFloat(Text.c_str(), &Value, Step);
    }
};
struct Combo {
    std::string Text = "Seçim";  std::vector<std::string> Items;  int Selected = 0;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG — SetNextItemWidth)
    Combo() {}
    Combo(const std::vector<std::string>& it) : Items(it) {}
    bool Draw() {
        const char* cur = (Selected >= 0 && Selected < (int)Items.size()) ? Items[Selected].c_str() : "";
        if (Width > 0) ImGui::SetNextItemWidth(Width);
        if (ImGui::BeginCombo(Text.c_str(), cur)) {
            for (int i = 0; i < (int)Items.size(); i++) {
                bool sel = (i == Selected);
                if (ImGui::Selectable(Items[i].c_str(), sel) && !sel) { Selected = i; ImGui::EndCombo(); return true; }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return false;
    }
};
struct ListBox {
    std::string Text = "Liste";  std::vector<std::string> Items;  int Selected = 0;  int HeightItems = 5;
    ImVec2 Size = ImVec2(0, 0);   // tasarım boyutu — y<=0: HeightItems'ten otomatik (WYSIWYG)
    bool Draw() {
        ImVec2 sz = Size;
        if (sz.y <= 0.0f)
            sz.y = ImGui::GetTextLineHeightWithSpacing() * (HeightItems + 0.25f) + ImGui::GetStyle().FramePadding.y * 2.0f;
        if (ImGui::BeginListBox(Text.c_str(), sz)) {
            for (int i = 0; i < (int)Items.size(); i++) {
                if (ImGui::Selectable(Items[i].c_str(), i == Selected)) Selected = i;
            }
            ImGui::EndListBox();
        }
        return false;
    }
};
struct ColorEdit4 {
    std::string Text = "Renk";  ImVec4 Col = ImVec4(1, 1, 1, 1);
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG — girişli kip)
    bool NoInputs = true;   // true: yalnız renk karesi (canvas ile aynı kip)
    bool Draw() {
        if (Width > 0 && !NoInputs) ImGui::SetNextItemWidth(Width);
        return ImGui::ColorEdit4(Text.c_str(), &Col.x, NoInputs ? ImGuiColorEditFlags_NoInputs : 0);
    }
};
struct ProgressBar {
    float Value = 0.5f;  std::string Overlay;  ImVec2 Size = ImVec2(200, 18);
    void Draw() {
        char buf[256]; buf[0] = 0;
        if (!Overlay.empty()) snprintf(buf, sizeof buf, "%s", Overlay.c_str());
        ImGui::ProgressBar(Value, Size, buf[0] ? buf : nullptr);
    }
};
// BOYUTLU AYRAÇ — W×H dolgu çubuğu (ImGuiCol_Separator rengiyle). Editör canvas'ı
// AYNI çubuğu çizer (WYSIWYG). "Kullanıcı: seperator ince çubuk gibi, büyütüp
// küçültemiyorum" — artık gerçek boyutlandırılabilir ayraç.
struct Separator {
    ImVec2 Size = ImVec2(200, 2);
    void Draw() {
        ImGui::Dummy(Size);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                          ImGui::GetColorU32(ImGuiCol_Separator));
    }
};
// BOYUTLU METİN AYRAÇ — metin + w'ye kadar çizgi. ImGui::SeparatorText mevcut
// pencere genişliğine yayıldığından w'lik sıfır-dolgu child'ına alınır.
struct SeparatorText {
    std::string Text = "Bölüm";  float Width = 0;   // 0 = mevcut genişlik
    bool Wrapped = false;
    void Draw() {
        if (Width <= 0) { ImGui::SeparatorText(Text.c_str()); return; }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild((Text + "_stw").c_str(), ImVec2(Width, 0.0f), ImGuiChildFlags_AutoResizeY);
        ImGui::SeparatorText(Text.c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
};
// BOYUTLU MADDE METNİ — BulletText kaydırmayı yok sayar: genişlik etiketi
// w'ye sığacak şekilde kısaltır (editör canvas'ı aynı FitTextToWidth'ı kullanır).
struct BulletText {
    std::string Text = "Madde";  float Width = 0;   // 0 = içerik genişliği
    void Draw() {
        std::string shown = Text;
        if (Width > 0)
            shown = FitTextToWidth(Text, Width - (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.x * 2.0f));
        ImGui::BulletText("%s", shown.c_str());
    }
};
struct PlotLines {
    std::string Text = "Grafik";  std::vector<float> Data;  float ScaleMin = 0, ScaleMax = 1; ImVec2 Size = ImVec2(280, 120);
    void Draw() {
        ImGui::PlotLines(Text.c_str(), Data.data(), (int)Data.size(), 0, nullptr, ScaleMin, ScaleMax, Size);
    }
};
struct PlotHistogram {
    std::string Text = "Histogram";  std::vector<float> Data;  ImVec2 Size = ImVec2(280, 120);
    void Draw() { ImGui::PlotHistogram(Text.c_str(), Data.data(), (int)Data.size(), 0, nullptr, FLT_MAX, FLT_MAX, Size); }
};
// konteynerler
struct ChildWindow {
    std::string Text = "Child";  ImVec2 Size = ImVec2(300, 200);  bool Border = true;
    bool Begin() {
        ImGuiChildFlags cf = Border ? ImGuiChildFlags_Borders : 0;
        return ImGui::BeginChild(Text.c_str(), Size, cf);
    }
    void End() { ImGui::EndChild(); }
};
struct GroupBox {
    std::string Text = "Grup";  std::string Id;  ImVec2 Size = ImVec2(320, 160);
    bool Begin() {
        // WinForms GroupBox görünümü: çerçeveli child + üst kenarda başlık
        // Id boşsa Text+"_grp" — editör üretimi benzersiz Id bağlar
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::BeginChild(Id.empty() ? (Text + "_grp").c_str() : (Id + "_grp").c_str(), Size, ImGuiChildFlags_Borders);
        return true;
    }
    void End() {
        // çocuklar kapandı: çerçeve + başlık çiz (GetItemRectMin/Max child'in)
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImVec2 rmax = ImGui::GetItemRectMax();
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(rmin, rmax, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
        ImVec2 ts = ImGui::CalcTextSize(Text.c_str());
        ImVec2 lp = ImVec2(rmin.x + 8, rmin.y - ts.y * 0.5f);
        dl->AddRectFilled(ImVec2(lp.x - 4, lp.y - 1), ImVec2(lp.x + ts.x + 4, lp.y + ts.y + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));
        dl->AddText(lp, ImGui::GetColorU32(ImGuiCol_Text), Text.c_str());
    }
};
struct Panel {
    // WinForms Panel karşılığı: kenarlıklı child bölge (bağımsız kaydırılabilir
    // yüzey). Text yalnızca benzersiz child kimliği için ("Görünür##panel1").
    std::string Text = "Panel";  ImVec2 Size = ImVec2(300, 200);
    bool Border = true;  float Rounding = -1.0f;
    bool Begin() {
        if (Rounding >= 0.0f) ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Rounding);
        ImGuiChildFlags cf = Border ? ImGuiChildFlags_Borders : 0;
        return ImGui::BeginChild(Text.c_str(), Size, cf);
    }
    void End() {
        ImGui::EndChild();
        if (Rounding >= 0.0f) ImGui::PopStyleVar();
    }
};
struct CollapsingHeader {
    std::string Text = "Başlık";  bool DefaultOpen = true;
    float Width = 0;   // >0: başlık bu genişlikte sınırlanır (child-clip — WYSIWYG)
    bool Wrapped = false, WasOpen = false;
    bool Begin() {
        if (Width > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild((Text + "_hcw").c_str(), ImVec2(Width, 0.0f), ImGuiChildFlags_AutoResizeY);
            Wrapped = true;
        }
        ImGuiTreeNodeFlags fl = DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        WasOpen = ImGui::CollapsingHeader(Text.c_str(), fl);
        return WasOpen;
    }
    // HER ZAMAN çağrılır (üretim: if (Begin()){çocuklar} End();) — child dengesi
    void End() { if (Wrapped) { ImGui::EndChild(); ImGui::PopStyleVar(); Wrapped = false; } }
};
struct TreeNode {
    std::string Text = "Düğüm";  bool DefaultOpen = false;
    float Width = 0;   // >0: düğüm bu genişlikte sınırlanır (child-clip — WYSIWYG)
    bool Wrapped = false, WasOpen = false;
    bool Begin() {
        if (Width > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild((Text + "_ncw").c_str(), ImVec2(Width, 0.0f), ImGuiChildFlags_AutoResizeY);
            Wrapped = true;
        }
        ImGuiTreeNodeFlags fl = DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        WasOpen = ImGui::TreeNodeEx(Text.c_str(), fl);
        return WasOpen;
    }
    // HER ZAMAN çağrılır — TreePop yalnız açıksa (dengeli)
    void End() {
        if (WasOpen) ImGui::TreePop();
        if (Wrapped) { ImGui::EndChild(); ImGui::PopStyleVar(); Wrapped = false; }
    }
};
struct TabBar {
    // BOYUT (WYSIWYG): sekmeler W×H sıfır-dolgu child bölgesine çizilir —
    // genişlik VE yükseklik tasarım boyutu. Editör canvas'ı aynı sarmalama.
    std::string Text = "Sekmeler";  ImVec2 Size = ImVec2(0, 0);
    bool BarOk = false;
    bool Begin() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild((Text + "_tbw").c_str(), Size, ImGuiChildFlags_None);
        BarOk = ImGui::BeginTabBar((Text + "_tb").c_str(), ImGuiTabBarFlags_None);
        return BarOk;
    }
    void End() {
        if (BarOk) ImGui::EndTabBar();
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
};
struct ModalPopup {
    std::string Text = "Pencere";  bool OpenReq = false;
    void Open() { OpenReq = true; }
    void Close() { OpenReq = false; ImGui::CloseCurrentPopup(); }
    bool Begin() {
        if (OpenReq && !ImGui::IsPopupOpen(Text.c_str()))
            ImGui::OpenPopup(Text.c_str());
        OpenReq = false;
        return ImGui::BeginPopupModal(Text.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    }
    void End() { ImGui::EndPopup(); }
    void Draw() {
        if (Begin()) {
            End();
        }
    }
};
struct Table {
    std::string Text = "Tablo";  int Columns = 3, Rows = 3;
    std::vector<std::string> Headers;
    float Width = 0;   // >0: tasarımdan genişlik (WYSIWYG — outer_size.x)
    void Draw() {
        ImVec2 outer(Width > 0 ? Width : 0.0f, 0.0f);
        if (ImGui::BeginTable((Text + "_tbl").c_str(), Columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, outer)) {
            for (auto& h : Headers) ImGui::TableSetupColumn(h.c_str());
            ImGui::TableHeadersRow();
            for (int r = 0; r < Rows; r++) {
                ImGui::TableNextRow();
                for (int cidx = 0; cidx < Columns; cidx++) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("-");
                }
            }
            ImGui::EndTable();
        }
    }
};
struct MenuBarWrap {
    std::vector<std::string> Menus;                  // "Dosya", "Düzen"...
    std::vector<std::string> Commands;               // "Dosya: Çıkış" biçiminde
    std::string LastCommand;                          // son tıklanan komut (MenuClick'te)
    bool Draw() {
        bool clicked = false;
        if (ImGui::BeginMenuBar()) {
            for (auto& m : Menus) {
                if (ImGui::BeginMenu(m.c_str())) {
                    for (auto& cmd : Commands) {
                        size_t c = cmd.find(':');
                        if (c == std::string::npos) continue;
                        std::string owner = cmd.substr(0, c);
                        std::string item = TrimA2(cmd.substr(c + 1));
                        if (owner != m) continue;
                        if (ImGui::MenuItem(item.c_str())) { LastCommand = m + ":" + item; clicked = true; }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenuBar();
        }
        return clicked;
    }
    static std::string TrimA2(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && (s[a] == ' ')) a++;
        while (b > a && (s[b-1] == ' ')) b--;
        return s.substr(a, b - a);
    }
};
struct FontRef {
    std::string Name = "Arial";  float Size = 18;  bool Bold = false;
    ImFont* Resolve() {
        ImGuiIO& io = ImGui::GetIO();
        ImFontAtlas* atlas = io.Fonts;
        ImFont* f = atlas ? atlas->AddFontFromFileTTF(Name.c_str(), Size) : nullptr;
        return f;
    }
};
// —— bileşenler ---------------------------------------------------------------
struct Timer {
    int Interval = 1000;     // ms
    bool Enabled = false;
    std::function<void()> Tick;
    double accum = 0.0;
    void FrameTick(float dtSec) {
        if (!Enabled || Interval <= 0) return;
        accum += dtSec * 1000.0;
        while (accum >= (double)Interval) {
            accum -= (double)Interval;
            if (Tick) Tick();
        }
    }
};
// —— form temeli + uygulama kütüğü --------------------------------------------
struct FormBase {
    bool Loaded = false;
    bool Visible = false;
    std::vector<Timer*> Timers;
    virtual ~FormBase() {}
    virtual void Render() = 0;
    virtual void OnLoad() {}
    void TickTimers(float dtSec) {
        for (auto* t : Timers) t->FrameTick(dtSec);
    }
    void Show() { Visible = true; Loaded = false; }
    void Hide() { Visible = false; }
};
struct App {
    std::vector<FormBase*> Forms;
    void Register(FormBase* f) { Forms.push_back(f); }
    void Frame(float dtSec) {
        for (auto* f : Forms) {
            if (!f->Visible) continue;
            if (!f->Loaded) { f->OnLoad(); f->Loaded = true; }
            f->TickTimers(dtSec);
            f->Render();
        }
    }
    bool AnyVisible() { for (auto* f : Forms) if (f->Visible) return true; return false; }
};
extern App* g_app;

// —— fontlar: fonts/DejaVuSans.ttf (exe yanı/üstleri) — Türkçe destek ------
inline std::string ProbeFontA(const char* rel) {
    char exeDir[MAX_PATH];
    GetModuleFileNameA(nullptr, exeDir, MAX_PATH);
    char* slash = strrchr(exeDir, '\\');
    if (slash) *slash = 0;
    const char* affixes[] = { "", "\\..", "\\..\\..", "\\..\\..\\.." };
    for (auto& a : affixes) {
        std::string p = std::string(exeDir) + a + "\\" + rel;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        std::string p = std::string(cwd) + "\\" + rel;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    return "";
}
inline void SetupFonts() {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = { 0x0020, 0x01FF, 0x2010, 0x205F, 0x2190, 0x21FF, 0 };  // Latin-1 + Ext-A + oklar
    std::string sans = ProbeFontA("fonts\\DejaVuSans.ttf");
    if (!sans.empty()) io.Fonts->AddFontFromFileTTF(sans.c_str(), 17.0f, nullptr, ranges);
    else io.Fonts->AddFontDefault();
    std::string mono = ProbeFontA("fonts\\DejaVuSansMono.ttf");
    if (!mono.empty()) io.FontDefault = io.Fonts->AddFontFromFileTTF(mono.c_str(), 15.0f, nullptr, ranges);
    io.Fonts->Build();
}

// —— tema paletleri (üretilen kod ile aynı isimler) ---------------------------
void ApplyGodOpsTheme();
void ApplyMidnightSteelTheme();
void ApplyAmmoTanTheme();

} // namespace IGOD
)IGODSDK";
}

// ---------------------------------------------------------------------------
//  main.cpp üretimi — seçilen backend'e göre
static const char* kBackendMain[] = {
// ---- dx11 (varsayılan) ----
R"MAINDX11(// main.cpp — IMGUI_GOD_SSSTIER üretimi (DirectX 11)
#include "Global.h"
#include "Form1.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static UINT g_rw = 0, g_rh = 0;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRTV() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void CleanupRTV() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }
static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL fls[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, fls, 2,
        D3D11_SDK_VERSION, &scd, &g_swap, &g_dev, &fl, &g_ctx);
    if (hr != S_OK)
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, fls, 2,
            D3D11_SDK_VERSION, &scd, &g_swap, &g_dev, &fl, &g_ctx);
    if (hr != S_OK) return false;
    CreateRTV();
    return true;
}
static void CleanupDevice() {
    CleanupRTV();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_rw = LOWORD(lParam); g_rh = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
IGOD::App* IGOD::g_app = nullptr;   // Global.h extern tanımı (file scope)
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGodApp", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — Uygulama", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDevice(hwnd)) { MessageBoxW(hwnd, L"D3D11 cihazı oluşturulamadı.", L"Hata", MB_OK | MB_ICONERROR); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    __THEMECALL__
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    IGOD::App app;
    IGOD::g_app = &app;
    Form1 form1;             // başlangıç formu (listeden ilk)
    app.Register(&form1);
    form1.Show();

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (g_rw != 0 && g_rh != 0) {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            CleanupRTV();
            g_swap->ResizeBuffers(0, g_rw, g_rh, DXGI_FORMAT_UNKNOWN, 0);
            g_rw = g_rh = 0;
            CreateRTV();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!app.AnyVisible()) PostQuitMessage(0);
        app.Frame(io.DeltaTime);
        ImGui::Render();
        const float clear[] = { 0.08f, 0.09f, 0.12f, 1.00f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
)MAINDX11",
// ---- dx9 ----
R"MAINDX9(// main.cpp — IMGUI_GOD_SSSTIER üretimi (DirectX 9)
#include "Global.h"
#include "Form1.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx9.h"
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

static LPDIRECT3D9 g_d3d = nullptr;
static LPDIRECT3DDEVICE9 g_dev = nullptr;
static D3DPRESENT_PARAMETERS g_pp = {};
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static bool CreateDeviceD3D(HWND hwnd) {
    if ((g_d3d = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr) return false;
    ZeroMemory(&g_pp, sizeof g_pp);
    g_pp.Windowed = TRUE;
    g_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_pp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_pp.EnableAutoDepthStencil = TRUE;
    g_pp.AutoDepthStencilFormat = D3DFMT_D16;
    g_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_pp, &g_dev) < 0)
        if (g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_pp, &g_dev) < 0)
            return false;
    return true;
}
static void CleanupDevice() {
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
    if (g_d3d) { g_d3d->Release(); g_d3d = nullptr; }
}
static void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (g_dev->Reset(&g_pp) == D3DERR_INVALIDCALL) IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE: if (wParam == SIZE_MINIMIZED) return 0; ResetDevice(); return 0;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
IGOD::App* IGOD::g_app = nullptr;   // Global.h extern tanımı (file scope)
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGodApp", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — Uygulama", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { MessageBoxW(hwnd, L"D3D9 cihazı oluşturulamadı.", L"Hata", MB_OK | MB_ICONERROR); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    __THEMECALL__
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_dev);

    IGOD::App app;
    IGOD::g_app = &app;
    Form1 form1;
    app.Register(&form1);
    form1.Show();

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!app.AnyVisible()) PostQuitMessage(0);
        app.Frame(io.DeltaTime);
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        g_dev->Present(nullptr, nullptr, nullptr, nullptr);
    }
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
)MAINDX9",
// ---- dx10 ----
R"MAINDX10(// main.cpp — IMGUI_GOD_SSSTIER üretimi (DirectX 10)
#include "Global.h"
#include "Form1.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx10.h"
#include <d3d10_1.h>
#include <d3d10.h>
#include <dxgi.h>
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3dcompiler.lib")

static ID3D10Device* g_dev = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D10RenderTargetView* g_rtv = nullptr;
static UINT g_rw = 0, g_rh = 0;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void CreateRTV() {
    ID3D10Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_dev->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
static void CleanupRTV() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }
static bool CreateDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT flags = 0;
    D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, flags, D3D10_SDK_VERSION, &scd, &g_swap, &g_dev);
    if (!g_dev) return false;
    CreateRTV();
    return true;
}
static void CleanupDevice() {
    CleanupRTV();
    if (g_swap) { g_swap->Release(); g_swap = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_rw = LOWORD(lParam); g_rh = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
IGOD::App* IGOD::g_app = nullptr;   // Global.h extern tanımı (file scope)
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGodApp", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — Uygulama", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { MessageBoxW(hwnd, L"D3D10 cihazı oluşturulamadı.", L"Hata", MB_OK | MB_ICONERROR); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    __THEMECALL__
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX10_Init(g_dev);

    IGOD::App app;
    IGOD::g_app = &app;
    Form1 form1;
    app.Register(&form1);
    form1.Show();

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        if (g_rw != 0 && g_rh != 0) {
            ImGui_ImplDX10_InvalidateDeviceObjects();
            CleanupRTV();
            g_swap->ResizeBuffers(0, g_rw, g_rh, DXGI_FORMAT_UNKNOWN, 0);
            g_rw = g_rh = 0;
            CreateRTV();
        }
        ImGui_ImplDX10_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!app.AnyVisible()) PostQuitMessage(0);
        app.Frame(io.DeltaTime);
        ImGui::Render();
        const float clear[] = { 0.08f, 0.09f, 0.12f, 1.00f };
        g_dev->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_dev->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }
    ImGui_ImplDX10_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
)MAINDX10",
// ---- dx12 ----
    #include "main_dx12_template.inc"
    ,
// ---- opengl3 ----
R"MAINGL3(// main.cpp — IMGUI_GOD_SSSTIER üretimi (OpenGL 3)
#include "Global.h"
#include "Form1.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <tchar.h>
#include <windows.h>
#pragma comment(lib, "opengl32.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static HDC g_hdc; static HGLRC g_hrc;
static bool CreateGL(HWND hwnd) {
    g_hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int fmt = ChoosePixelFormat(g_hdc, &pfd);
    if (!fmt) return false;
    if (!SetPixelFormat(g_hdc, fmt, &pfd)) return false;
    g_hrc = wglCreateContext(g_hdc);
    if (!g_hrc) return false;
    if (!wglMakeCurrent(g_hdc, g_hrc)) return false;
    return true;
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
IGOD::App* IGOD::g_app = nullptr;   // Global.h extern tanımı (file scope)
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, L"IMGodApp", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0L, wc.lpszClassName, L"IMGUI GOD SSS TIER — Uygulama", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateGL(hwnd)) { MessageBoxW(hwnd, L"OpenGL bağlamı oluşturulamadı.", L"Hata", MB_OK | MB_ICONERROR); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    IGOD::SetupFonts();   // fonts/DejaVu*.ttf — Türkçe karakter desteği
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    __THEMECALL__
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    IGOD::App app;
    IGOD::g_app = &app;
    Form1 form1;
    app.Register(&form1);
    form1.Show();

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (!app.AnyVisible()) PostQuitMessage(0);
        app.Frame(io.DeltaTime);
        ImGui::Render();
        int w, h; RECT r; GetClientRect(hwnd, &r); w = r.right; h = r.bottom;
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.09f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SwapBuffers(g_hdc);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    wglMakeCurrent(nullptr, nullptr);
    if (g_hrc) wglDeleteContext(g_hrc);
    if (g_hdc) ReleaseDC(hwnd, g_hdc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
)MAINGL3",
};

std::string EmitMainCpp() {
    std::string s = kBackendMain[g_settings.backend];
    std::string themeCall = theme::GenThemeApiCall(g_settings.themeId);
    size_t p;
    while ((p = s.find("__THEMECALL__")) != std::string::npos)
        s.replace(p, 13, themeCall.empty() ? "ImGui::StyleColorsDark();" : "    " + themeCall);
    return s;
}

// ---------------------------------------------------------------------------
//  .sln / .vcxproj / mingw betiği
std::string EmitSln(const std::string& pn) {
    std::string guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942"; // VC++ proje tipi
    std::ostringstream o;
    o << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
    o << "# Visual Studio Version 16\nVisualStudioVersion = 16.0.30330.148\n";
    o << "Project(\"{" << guid << "}\") = \"" << pn << "\", \"" << pn << "\\" << pn << ".vcxproj\", \"{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}\"\nEndProject\n";
    o << "Global\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
    o << "\t\tDebug|x64 = Debug|x64\n\t\tRelease|x64 = Release|x64\n";
    o << "\t\tDebug|x86 = Debug|x86\n\t\tRelease|x86 = Release|x86\n";
    o << "\tEndGlobalSection\n";
    o << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Debug|x64.ActiveCfg = Debug|x64\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Debug|x64.Build.0 = Debug|x64\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Release|x64.ActiveCfg = Release|x64\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Release|x64.Build.0 = Release|x64\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Debug|x86.ActiveCfg = Debug|Win32\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Debug|x86.Build.0 = Debug|Win32\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Release|x86.ActiveCfg = Release|Win32\n";
    o << "\t\t{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}.Release|x86.Build.0 = Release|Win32\n";
    o << "\tEndGlobalSection\n";
    o << "\tGlobalSection(SolutionProperties) = preSolution\n\t\tHideSolutionNode = FALSE\n\tEndGlobalSection\n";
    o << "EndGlobal\n";
    return o.str();
}

static std::string BackendLibsVcx() {
    switch ((EditorBackend)g_settings.backend) {
    case EditorBackend::DX9: return "d3d9.lib";
    case EditorBackend::DX10: return "d3d10.lib;d3dcompiler.lib";
    case EditorBackend::DX11: return "d3d11.lib;dxgi.lib;d3dcompiler.lib";
    case EditorBackend::DX12: return "d3d12.lib;dxgi.lib;d3dcompiler.lib";
    case EditorBackend::OPENGL3: return "opengl32.lib";
    }
    return "d3d11.lib;dxgi.lib";
}
static std::string BackendImpl() {
    switch ((EditorBackend)g_settings.backend) {
    case EditorBackend::DX9: return "imgui_impl_dx9.cpp";
    case EditorBackend::DX10: return "imgui_impl_dx10.cpp";
    case EditorBackend::DX11: return "imgui_impl_dx11.cpp";
    case EditorBackend::DX12: return "imgui_impl_dx12.cpp";
    case EditorBackend::OPENGL3: return "imgui_impl_opengl3.cpp";
    }
    return "imgui_impl_dx11.cpp";
}

std::string EmitVcxproj(const std::string& pn) {
    std::ostringstream o;
    o << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    o << "<Project DefaultTargets=\"Build\" ToolsVersion=\"16.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
    o << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
    o << "    <ProjectConfiguration Include=\"Debug|x64\">\n      <Configuration>Debug</Configuration>\n      <Platform>x64</Platform>\n    </ProjectConfiguration>\n";
    o << "    <ProjectConfiguration Include=\"Release|x64\">\n      <Configuration>Release</Configuration>\n      <Platform>x64</Platform>\n    </ProjectConfiguration>\n";
    o << "    <ProjectConfiguration Include=\"Debug|Win32\">\n      <Configuration>Debug</Configuration>\n      <Platform>Win32</Platform>\n    </ProjectConfiguration>\n";
    o << "    <ProjectConfiguration Include=\"Release|Win32\">\n      <Configuration>Release</Configuration>\n      <Platform>Win32</Platform>\n    </ProjectConfiguration>\n";
    o << "  </ItemGroup>\n";
    o << "  <PropertyGroup Label=\"Globals\">\n    <ProjectGuid>{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}</ProjectGuid>\n";
    o << "    <RootNamespace>" << pn << "</RootNamespace>\n    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n  </PropertyGroup>\n";
    o << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
    auto Cfg = [&](const char* cfg, const char* plat, bool debug) {
        o << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='" << cfg << "|" << plat << "'\" Label=\"Configuration\">\n";
        o << "    <ConfigurationType>Application</ConfigurationType>\n    <UseDebugLibraries>" << (debug ? "true" : "false") << "</UseDebugLibraries>\n";
        o << "    <PlatformToolset>v142</PlatformToolset>\n    <CharacterSet>Unicode</CharacterSet>\n  </PropertyGroup>\n";
    };
    Cfg("Debug", "Win32", true); Cfg("Release", "Win32", false);
    Cfg("Debug", "x64", true); Cfg("Release", "x64", false);
    o << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
    auto Link = [&](const char* cfg, const char* plat, const char* outdir) {
        o << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='" << cfg << "|" << plat << "'\">\n";
        o << "    <ClCompile>\n      <WarningLevel>Level3</WarningLevel>\n      <LanguageStandard>stdcpp17</LanguageStandard>\n      <PreprocessorDefinitions>WIN32_LEAN_AND_MEAN;_CRT_SECURE_NO_WARNINGS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n    </ClCompile>\n";
        o << "    <Link>\n      <SubSystem>Windows</SubSystem>\n      <AdditionalDependencies>" << BackendLibsVcx() << ";%(AdditionalDependencies)</AdditionalDependencies>\n    </Link>\n";
        o << "    <PostBuildEvent>\n      <Command>xcopy /E /I /Y \"$(ProjectDir)imgui\" \"$(OutDir)imgui\" 2&gt;nul\nexit 0</Command>\n    </PostBuildEvent>\n";
        o << "  </ItemDefinitionGroup>\n";
    };
    Link("Debug", "Win32", "Debug"); Link("Release", "Win32", "Release");
    Link("Debug", "x64", "Debug"); Link("Release", "x64", "Release");
    // kaynaklar
    o << "  <ItemGroup>\n";
    o << "    <ClCompile Include=\"main.cpp\" />\n";
    for (auto& f : forms::All()) o << "    <ClCompile Include=\"" << f.name << ".cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\imgui.cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\imgui_draw.cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\imgui_tables.cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\imgui_widgets.cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\imgui_demo.cpp\" />\n";
    o << "    <ClCompile Include=\"imgui\\backends\\" << BackendImpl() << "\" />\n";
    o << "    <ClCompile Include=\"imgui\\backends\\imgui_impl_win32.cpp\" />\n";
    o << "  </ItemGroup>\n";
    o << "  <ItemGroup>\n";
    auto Hdr = [&](const char* n) { o << "    <ClInclude Include=\"" << n << "\" />\n"; };
    Hdr("Global.h");
    for (auto& f : forms::All()) Hdr((f.name + ".h").c_str());
    Hdr("imgui\\imgui.h"); Hdr("imgui\\imconfig.h"); Hdr("imgui\\imgui_internal.h");
    o << "  </ItemGroup>\n";
    o << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n</Project>\n";
    return o.str();
}

std::string EmitMingwScript(const std::string& pn) {
    const char* backendFlag = "-ld3d11 -ldxgi -ld3dcompiler";
    const char* implFile = "imgui_impl_dx11.cpp";
    switch ((EditorBackend)g_settings.backend) {
    case EditorBackend::DX9: backendFlag = "-ld3d9 -ld3dcompiler"; implFile = "imgui_impl_dx9.cpp"; break;
    case EditorBackend::DX10: backendFlag = "-ld3d10 -ld3dcompiler -ldxgi"; implFile = "imgui_impl_dx10.cpp"; break;
    case EditorBackend::DX11: backendFlag = "-ld3d11 -ldxgi -ld3dcompiler -ldwmapi"; implFile = "imgui_impl_dx11.cpp"; break;
    case EditorBackend::DX12: backendFlag = "-ld3d12 -ldxgi -ld3dcompiler"; implFile = "imgui_impl_dx12.cpp"; break;
    case EditorBackend::OPENGL3: backendFlag = "-lopengl32"; implFile = "imgui_impl_opengl3.cpp"; break;
    }
    std::ostringstream o;
    o << "#!/bin/bash\n# IMGUI_GOD_SSSTIER üretimi — MinGW/GCC/clang bağımsız derleme (x64)\n";
    o << "set -e\ncd \"$(dirname \"$0\")\"\nmkdir -p Bin\n";
    o << "CXX=${CXX:-}\n";
    o << "if [ -z \"$CXX\" ]; then\n";
    o << "  for c in x86_64-w64-mingw32-clang++ x86_64-w64-mingw32-g++-posix x86_64-w64-mingw32-g++ i686-w64-mingw32-g++; do\n";
    o << "    command -v \"$c\" >/dev/null 2>&1 && CXX=\"$c\" && break\n";
    o << "  done\n";
    o << "fi\n";
    o << "[ -n \"$CXX\" ] || { echo 'MinGW derleyicisi bulunamadı (x86_64-w64-mingw32-g++)'; exit 1; }\n";
    o << "SRCS=\"main.cpp";
    for (auto& f : forms::All()) o << " " << f.name << ".cpp";
    o << " imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp";
    o << " imgui/backends/" << implFile << " imgui/backends/imgui_impl_win32.cpp\"\n";
    o << "\"$CXX\" -O2 -std=c++17 -municode -mwindows -static -o Bin/" << pn << ".exe $SRCS "
      << " -I. -Iimgui -Iimgui/backends " << backendFlag
      << " -luuid -lole32 -lgdi32 -lwinmm -limm32\n";
    o << "echo OK: Bin/" << pn << ".exe\n";
    return o.str();
}

// ---------------------------------------------------------------------------
//  ImGui SDK kopyalama (exe yanındaki imgui/ klasöründen)
static std::string LowerFirst(std::string s) {
    if (!s.empty()) s[0] = (char)tolower((unsigned char)s[0]);
    return s;
}
bool CopyImGuiSdk(const std::string& destRoot) {
    // ImGui SDK konumu: exe yanı VEYA exe'nin üst klasörlerinde (Bin/.. gibi)
    std::string src;
    {
        std::vector<std::string> roots{ g_exeDirA };
        for (int up = 0; up < 4; up++) {
            size_t slash = roots.back().find_last_of("/\\");
            if (slash == std::string::npos) break;
            roots.push_back(roots.back().substr(0, slash));
        }
        roots.push_back(".");
        for (auto& r : roots) {
            std::string cand = JoinPathA(r, g_settings.imguiDir);
            if (FileExistsA(cand)) { src = cand; break; }
        }
    }
    if (src.empty()) return false;
    std::string dst = JoinPathA(destRoot, "imgui");
    if (!MkDirsA(dst) || !MkDirsA(JoinPathA(dst, "backends"))) return false;
    // kaynak backends düzeni: SDK (imgui/backends/) VEYA düz (editör imgui/ sınıfsız)
    std::string srcBack = JoinPathA(src, "backends");
    if (!FileExistsA(srcBack)) srcBack = src;
    const char* core[] = {
        "imgui.h", "imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp", "imgui_demo.cpp",
        "imgui_internal.h", "imconfig.h", "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h", "LICENSE.txt",
    };
    for (const char* f : core) {
        std::string s = JoinPathA(src, f), d = JoinPathA(dst, f);
        std::string data;
        if (ReadFileTextA(s, data) && !WriteFileTextA(d, data)) return false;
    }
    // backend dosyaları: seçilen backend + win32 (+ opengl3 için loader başlığı)
    std::vector<std::string> be{ BackendImpl(), "imgui_impl_win32.cpp" };
    for (auto& b : be) {
        std::string data;
        if (!ReadFileTextA(JoinPathA(srcBack, b), data)) return false;
        if (!WriteFileTextA(JoinPathA(JoinPathA(dst, "backends"), b), data)) return false;
        // eş .h dosyası
        std::string h = b.substr(0, b.size() - 4) + ".h";
        if (ReadFileTextA(JoinPathA(srcBack, h), data))
            WriteFileTextA(JoinPathA(JoinPathA(dst, "backends"), h), data);
    }
    if (g_settings.backend == (int)EditorBackend::OPENGL3) {
        std::string data;
        if (ReadFileTextA(JoinPathA(srcBack, "imgui_impl_opengl3_loader.h"), data))
            WriteFileTextA(JoinPathA(JoinPathA(dst, "backends"), "imgui_impl_opengl3_loader.h"), data);
    }
    return true;
}

// ---------------------------------------------------------------------------
//  TAM PROJE YAYINI
bool EmitProject(const std::string& folder) {
    forms::ParkActiveToModel();   // g_model → aktif form girdisi (üretim güncel veriyle)
    if (!MkDirsA(folder)) return false;
    std::string pn = g_settings.projectName;
    // klasör yapısı: <folder>/<pn>/ kaynaklar, <folder>/<pn>.sln
    std::string projDir = JoinPathA(folder, pn);
    std::string binDir = JoinPathA(projDir, "Bin");
    if (!MkDirsA(projDir) || !MkDirsA(binDir)) return false;

    // ImGui SDK kopyası
    if (!CopyImGuiSdk(projDir)) return false;
    // fonts/ kopyala (DejaVu — Türkçe karakter desteği; editör exe yanı/üstlerinden)
    {
        const char* cand[] = { "fonts/DejaVuSans.ttf", "fonts\\DejaVuSans.ttf" };
        std::string srcFont;
        // exe yanında ve üstlerinde ara
        std::vector<std::string> roots{ g_exeDirA };
        for (int up = 0; up < 3; up++) {
            std::string r = roots.back();
            size_t slash = r.find_last_of("/\\");
            if (slash == std::string::npos) break;
            roots.push_back(r.substr(0, slash));
        }
        roots.push_back(".");
        for (auto& r : roots) {
            std::string f = JoinPathA(r, "fonts/DejaVuSans.ttf");
            std::string data;
            if (ReadFileTextA(f, data)) { srcFont = r; break; }
        }
        if (!srcFont.empty()) {
            std::string dstFonts = JoinPathA(projDir, "fonts");
            MkDirsA(dstFonts);
            const char* names[] = { "DejaVuSans.ttf", "DejaVuSansMono.ttf" };
            for (auto& n2 : names) {
                std::string data;
                if (ReadFileTextA(JoinPathA(srcFont, std::string("fonts/") + n2), data))
                    WriteFileTextA(JoinPathA(dstFonts, n2), data);
            }
        }
    }


    // Global.h
    if (!WriteFileTextA(JoinPathA(projDir, "Global.h"), EmitGlobalH())) return false;
    // main.cpp
    if (!WriteFileTextA(JoinPathA(projDir, "main.cpp"), EmitMainCpp())) return false;
    // formlar
    for (auto& f : forms::All()) {
        if (!WriteFileTextA(JoinPathA(projDir, f.name + ".cpp"), EmitFormCpp(f.name, f.model))) return false;
        if (!WriteFileTextA(JoinPathA(projDir, f.name + ".h"), EmitFormH(f.name, f.model))) return false;
    }
    // ikincil formlar için başlatma: main.cpp'e ekleme (FormN.h include + Show çağrısı açıklaması)
    if (forms::Count() > 1) {
        std::string mainTxt;
        ReadFileTextA(JoinPathA(projDir, "main.cpp"), mainTxt);
        std::string incs, decls, regs;
        for (size_t i = 1; i < forms::Count(); i++) {
            auto& f = *forms::At(i);
            incs += "#include \"" + f.name + ".h\"\n";
            decls += "    " + f.name + " " + LowerFirst(f.name) + ";\n";
            regs += "    app.Register(&" + LowerFirst(f.name) + ");\n";
        }
        // include'ları Global.h'den sonra ekle
        size_t p = mainTxt.find("#include \"Form1.h\"");
        if (p != std::string::npos) mainTxt.insert(p, incs);
        p = mainTxt.find("    IGOD::App app;");
        if (p != std::string::npos) mainTxt.insert(p + strlen("    IGOD::App app;"), "\n" + decls + regs +
            "    // ikincil formları açmak için: " + LowerFirst(forms::At(1)->name) + ".Show();");
        WriteFileTextA(JoinPathA(projDir, "main.cpp"), mainTxt);
    }
    // .sln + .vcxproj
    if (g_settings.genVS2019) {
        if (!WriteFileTextA(JoinPathA(folder, pn + ".sln"), EmitSln(pn))) return false;
        if (!WriteFileTextA(JoinPathA(projDir, pn + ".vcxproj"), EmitVcxproj(pn))) return false;
    }
    // mingw betiği
    if (g_settings.genMinGW) {
        std::string sh = EmitMingwScript(pn);
        WriteFileTextA(JoinPathA(projDir, "build_mingw.sh"), sh);
    }
    // README
    std::string rd = "# " + pn + " — IMGUI_GOD_SSSTIER üretimi\n\n";
    rd += "Derleme:\n  * VS2019: " + pn + ".sln aç (v142, x64/Win32, Unicode)\n";
    rd += "  * MinGW: bash build_mingw.sh\n\n";
    rd += "Backend: " + std::string(BackendNameA((EditorBackend)g_settings.backend)) +
          " | Tema: " + theme::NameOf(g_settings.themeId) + "\n";
    rd += "Formlar: ";
    for (auto& f : forms::All()) rd += f.name + " ";
    rd += "\n";
    WriteFileTextA(JoinPathA(projDir, "README.md"), rd);
    return true;
}

} // namespace codegen
