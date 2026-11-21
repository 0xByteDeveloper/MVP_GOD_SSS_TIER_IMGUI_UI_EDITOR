#!/usr/bin/env python3
# codegen_patch.py — EmitCtrl tam yeniden yazımı (tasarım bağlama + stil + ##ad)
# + EmitFormCpp form stili + Global.h sarmalayıcı iyileştirmeleri
p = "/home/z/my-project/work/god_editor/src/codegen.cpp"
src = open(p, encoding="utf-8").read()

BS = "\\"

# ---- 0) include objstyle ----
if '#include "objstyle.h"' not in src:
    src = src.replace('#include "codegen.h"', '#include "codegen.h"\n#include "objstyle.h"', 1)

# ---- 1) EmitCtrl: baştan sona yeniden yaz --------------------------------
i0 = src.index("static void EmitCtrl(EmitCtx& ctx, const Ctrl* c) {")
i1 = src.index("// ---------------------------------------------------------------------------\n//  Form.cpp / Form.h", i0)

new_emit = r'''// tasarım özelliği → initlist bağlama satırı (WinForms InitializeComponent ruhu:
// değerler FORM YAPICISINDA bir kez bağlanır, kullanıcı kodu runtime'da serbestçe değiştirir)
static void Bind(EmitCtx& ctx, const Ctrl* c, const std::string& line) {
    ctx.initlist << "    " << c->name << "." << line << "\n";
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
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Checked0 = " << (c->GetB("Checked") ? "true" : "false") << ";\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "Checked") << "();\n";
        ctx.decls << "    IGOD::Checkbox " << c->name << ";   // tasarım: " << StrOf(c, "Text", "") << "\n";
    }
    else if (t == "RadioButton") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Seçim"));
        Bind(ctx, c, std::string("Active = ") + (c->GetB("Active") ? "true" : "false"));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "Click") << "();\n";
        ctx.decls << "    IGOD::RadioButton " << c->name << ";\n";
    }
    else if (t == "SliderFloat" || t == "SliderInt" || t == "DragFloat" || t == "DragInt" ||
             t == "InputText" || t == "InputTextMultiline" || t == "InputInt" || t == "InputFloat") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "InputText" || t == "InputTextMultiline" ? "Yazı" : "Değer"));
        if (t == "SliderFloat" || t == "DragFloat" || t == "InputFloat") {
            Bind(ctx, c, "Value = " + FNum((float)atof(NumOf(c, "Value", "0").c_str())) + "f");
            Bind(ctx, c, "Min = " + FNum((float)atof(NumOf(c, "Min", "0").c_str())) + "f");
            Bind(ctx, c, "Max = " + FNum((float)atof(NumOf(c, "Max", "100").c_str())) + "f");
        } else if (t == "SliderInt" || t == "DragInt" || t == "InputInt") {
            Bind(ctx, c, "Value = " + std::to_string((int)strtol(NumOf(c, "Value", "0").c_str(), nullptr, 10)));
            Bind(ctx, c, "Min = " + std::to_string((int)strtol(NumOf(c, "Min", "0").c_str(), nullptr, 10)));
            Bind(ctx, c, "Max = " + std::to_string((int)strtol(NumOf(c, "Max", "100").c_str(), nullptr, 10)));
        }
        if (t == "SliderFloat" || t == "SliderInt") Bind(ctx, c, "Format = " + UPlain(StrOf(c, "Format", t == "SliderFloat" ? "%.1f" : "%d")));
        if (t == "DragFloat" || t == "DragInt")    Bind(ctx, c, "Speed = " + FNum((float)atof(NumOf(c, "Speed", "1").c_str())) + "f");
        if (t == "InputInt" || t == "InputFloat")   Bind(ctx, c, "Step = " + FNum((float)atof(NumOf(c, "Step", "1").c_str())) + "f");
        if (t == "InputText") {
            Bind(ctx, c, "Value = " + UPlain(StrOf(c, "Value", "")));
            Bind(ctx, c, "Hint = " + UPlain(StrOf(c, "Hint", "")));
            Bind(ctx, c, std::string("Password = ") + (c->GetB("Password") ? "true" : "false"));
            Bind(ctx, c, std::string("ReadOnly = ") + (c->GetB("ReadOnly") ? "true" : "false"));
        }
        if (t == "InputTextMultiline") {
            Bind(ctx, c, "Value = " + UPlain(StrOf(c, "Value", "")));
            Bind(ctx, c, "ReadOnly = " + std::string(c->GetB("ReadOnly") ? "true" : "false"));
            Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "280").c_str(), nullptr, 10),
                                            c->h ? c->h : (int)strtol(NumOf(c, "Height", "120").c_str(), nullptr, 10)));
        }
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "ValueChanged") << "();\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";   // tasarım: " << StrOf(c, "Text", "") << "\n";
        if (t == "InputText") {
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
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Draw()) " << HandlerOf(c->name, "ColorChanged") << "();\n";
        ctx.decls << "    IGOD::ColorEdit4 " << c->name << ";\n";
    }
    else if (t == "ProgressBar") {
        Bind(ctx, c, "Value = " + FNum((float)atof(NumOf(c, "Value", "0.5").c_str())) + "f");
        Bind(ctx, c, "Overlay = " + UPlain(StrOf(c, "Overlay", "")));
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "200").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "18").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::ProgressBar " << c->name << ";\n";
    }
    else if (t == "Separator") {
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    ImGui::Separator();\n";
    }
    else if (t == "SeparatorText") {
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    ImGui::SeparatorText(" << ULabel(c, "Text", "Bölüm") << ");\n";
    }
    else if (t == "Text" || t == "TextColored" || t == "BulletText" || t == "LabelText") {
        // kimliksiz çıktılar — ## soneki GÖRÜNÜR, bu yüzden düz metin (çakışma yok)
        std::string s = EscCppUtf8(StrOf(c, "Text", "Etiket"));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        if (t == "Text") {
            if (c->GetB("Wrapped")) ctx.body << "    ImGui::TextWrapped(" << UPlain(StrOf(c, "Text", "Etiket")) << ");\n";
            else ctx.body << "    ImGui::Text(" << UPlain(StrOf(c, "Text", "Etiket")) << ");\n";
        }
        else if (t == "TextColored") {
            int r = (int)strtol(NumOf(c, "R", "255").c_str(), nullptr, 10);
            int g = (int)strtol(NumOf(c, "G", "200").c_str(), nullptr, 10);
            int b = (int)strtol(NumOf(c, "B", "80").c_str(), nullptr, 10);
            ctx.body << "    ImGui::TextColored(" << FVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f) << ", " << UPlain(StrOf(c, "Text", "Renkli etiket")) << ");\n";
        }
        else if (t == "BulletText") ctx.body << "    ImGui::BulletText(" << UPlain(StrOf(c, "Text", "Madde")) << ");\n";
        else ctx.body << "    ImGui::LabelText(" << ULabel(c, "Text", "Alan") << ", " << UPlain(StrOf(c, "Value", "değer")) << ");\n";
    }
    else if (t == "PlotLines" || t == "PlotHistogram") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", t == "PlotLines" ? "Grafik" : "Histogram"));
        std::string data;
        for (auto& s : SplitW(Utf8ToWideStr(StrOf(c, "Items", "")), L'|')) {
            if (!data.empty()) data += ", ";
            data += FNum((float)wcstod(s.c_str(), nullptr)) + "f";
        }
        if (data.empty()) data = "0.0f";
        Bind(ctx, c, "Data = { " + data + " }");
        if (t == "PlotLines") {
            Bind(ctx, c, "ScaleMin = " + FNum((float)atof(NumOf(c, "ScaleMin", "0").c_str())) + "f");
            Bind(ctx, c, "ScaleMax = " + FNum((float)atof(NumOf(c, "ScaleMax", "1").c_str())) + "f");
        }
        Bind(ctx, c, "Size = " + FVec2(c->w ? c->w : (int)strtol(NumOf(c, "Width", "280").c_str(), nullptr, 10),
                                        c->h ? c->h : (int)strtol(NumOf(c, "Height", "120").c_str(), nullptr, 10)));
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Draw();\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";\n";
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
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    if (" << c->name << ".Begin()) {\n";
        for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) EmitCtrl(ctx, ch);
        ctx.body << "        " << c->name << ".End();\n";
        ctx.body << "    }\n";
        ctx.decls << "    IGOD::" << t << " " << c->name << ";\n";
    }
    else if (t == "TabBar") {
        Bind(ctx, c, "Text = " + ULabel(c, "Text", "Sekmeler"));   // ## sonekli tabbar kimliği
        ctx.body << "    ImGui::SetCursorPos(" << pos << ");\n";
        ctx.body << "    " << c->name << ".Begin();\n";
        std::string items = StrOf(c, "Items", "Genel|Aayarlar");
        for (auto& s : SplitW(Utf8ToWideStr(items), L'|')) {
            std::string title = WideStrToUtf8(s);
            ctx.body << "    if (ImGui::BeginTabItem(" << UPlain(title) << ")) {\n";
            for (Ctrl* ch : m.ChildrenOf(const_cast<Ctrl*>(c))) {
                std::string owner = ch->Get("TabOwner", "");
                if (owner == title) EmitCtrl(ctx, ch);
            }
            ctx.body << "        ImGui::EndTabItem();\n    }\n";
        }
        ctx.body << "    " << c->name << ".End();\n";
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
        Bind(ctx, c, "Size = " + FNum((float)atof(NumOf(c, "Size", "18").c_str())) + "f");
        Bind(ctx, c, std::string("Bold = ") + (c->GetB("Bold") ? "true" : "false"));
        ctx.decls << "    IGOD::FontRef " << c->name << ";\n";
    }
    else {
        ctx.body << "    /* bilinmeyen nesne: " << t << " " << c->name << " */\n";
    }

    if (disabled) ctx.body << "    ImGui::EndDisabled();\n";
    if (!stPop.empty()) ctx.body << stPop;
}

'''
src = src[:i0] + new_emit + src[i1:]

# ---- 2) EmitFormCpp: form düzeyi stil push'ları ----
old_form_begin = '''    o << "//== IMGOD:INIT-BEGIN (edit\\303\\274retimi — elle de\\304\\237i\\305\\237tirmeyin)\\n";
    o << "void " << formName << "::Render() {\\n";
    o << "    ImGui::SetNextWindowSize(ImVec2(" << m.formW << ", " << m.formH << "), ImGuiCond_FirstUseEver);\\n";
    o << "    if (!ImGui::Begin(u8\\"" << EscCppUtf8(title) << "\\", nullptr, " << flags << ")) { ImGui::End(); return; }\\n";
'''
# ham metinde tam bul (escape'ler dosyada literal)
needle_begin = '    o << "void " << formName << "::Render() {\\n";\n    o << "    ImGui::SetNextWindowSize(ImVec2('
i = src.find(needle_begin)
assert i > 0, "form render anchor yok"
# bu noktadan itibaren Begin satırının sonuna kadar değiştir
j = src.index('    // üst-seviye nesneler', i)
seg = src[i:j]
old_line = '    o << "    if (!ImGui::Begin(u8\\\\" << EscCppUtf8(title) << "\\\\", nullptr, " << flags << ")) { ImGui::End(); return; }\\n";'
# seg içinde Begin satırını bul ve stil ekle
k = seg.index('ImGui::End(); return; }')
# k: Begin satırı içindeki konum — satır sonunu bul
kend = seg.index('\n', k) + 1
new_seg = (
    '    o << "void " << formName << "::Render() {\\n";\n'
    '    // form stili (varsa) — canvas ile aynı push dizisi\n'
    '    int _stylePushes = 0; (void)_stylePushes;\n'
    '    {\n'
    '        float f[4];\n'
    '        if (objstyle::ParseHexColor(m.formProps.count("WindowBgColor") ? m.formProps.at("WindowBgColor") : "", f)) {\n'
    '            o << "    ImGui::PushStyleColor(ImGuiCol_WindowBg, " << FVec4(f[0], f[1], f[2], f[3]) << ");\\n";\n'
    '        }\n'
    '        if (m.formProps.count("WindowRounding") && (float)atof(m.formProps.at("WindowRounding").c_str()) >= 0)\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, " << FNum((float)atof(m.formProps.at("WindowRounding").c_str())) << "f);\\n";\n'
    '        if (m.formProps.count("WindowBorderSize") && (float)atof(m.formProps.at("WindowBorderSize").c_str()) >= 0)\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, " << FNum((float)atof(m.formProps.at("WindowBorderSize").c_str())) << "f);\\n";\n'
    '    }\n'
    '    o << "    ImGui::SetNextWindowSize(ImVec2(" << m.formW << ", " << m.formH << "), ImGuiCond_FirstUseEver);\\n";\n'
)
new_seg += seg[seg.index('    o << "    ImGui::SetNextWindowSize'):kend] if False else ''
# Begin satırı: eskisini al
begin_line_start = seg.index('    o << "    if (!ImGui::Begin(')
begin_line_end = kend
old_begin_line = seg[begin_line_start:begin_line_end]
new_begin_line = old_begin_line.replace('{ ImGui::End(); return; }', '{ ImGui::End(); ' + '_popStyle; return; }')
# _popStyle tanımı: stil varsa pop satırları
new_seg += (
    '    std::string _popStyle;\n'
    '    if (m.formProps.count("WindowBgColor") && objstyle::ParseHexColor(m.formProps.at("WindowBgColor"), *(new float[4])) ) {}\n'
    '    // pop satırları: (aşağıda hesaplanır)\n'
)
# Basitleştirilmiş yaklaşım: yerine doğrudan yeniden kur
seg_new = (
    '    o << "void " << formName << "::Render() {\\n";\n'
    '    // ---- form stili (varsa) — canvas ile AYNI push dizisi ----\n'
    '    int _nStyle = 0;\n'
    '    {\n'
    '        float f[4];\n'
    '        if (objstyle::ParseHexColor(m.formProps.count("WindowBgColor") ? m.formProps.at("WindowBgColor") : "", f)) {\n'
    '            o << "    ImGui::PushStyleColor(ImGuiCol_WindowBg, " << FVec4(f[0], f[1], f[2], f[3]) << ");\\n";\n'
    '            _nStyle++;\n'
    '        }\n'
    '        if (m.formProps.count("WindowRounding") && (float)atof(m.formProps.at("WindowRounding").c_str()) >= 0) {\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, " << FNum((float)atof(m.formProps.at("WindowRounding").c_str())) << "f);\\n";\n'
    '            _nStyle++;\n'
    '        }\n'
    '        if (m.formProps.count("WindowBorderSize") && (float)atof(m.formProps.at("WindowBorderSize").c_str()) >= 0) {\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, " << FNum((float)atof(m.formProps.at("WindowBorderSize").c_str())) << "f);\\n";\n'
    '            _nStyle++;\n'
    '        }\n'
    '    }\n'
    '    o << "    ImGui::SetNextWindowSize(ImVec2(" << m.formW << ", " << m.formH << "), ImGuiCond_FirstUseEver);\\n";\n'
    '    o << "    if (!ImGui::Begin(u8\\"" << EscCppUtf8(title) << "\\", nullptr, " << flags << ")) { ImGui::End();\n";\n'
    '    if (_nStyle > 0) o << "        ImGui::PopStyleVar/Color hesaplı — aşağıda\\n";\n'
)
# Bu yaklaşım karmaşıklaşıyor — temiz çözüm: segmenti komple elle yaz
seg_final = (
    '    o << "void " << formName << "::Render() {\\n";\n'
    '    // ---- form stili (varsa) — canvas ile AYNI push dizisi (WYSIWYG) ----\n'
    '    int _nStyleCol = 0, _nStyleVar = 0;\n'
    '    {\n'
    '        float f[4];\n'
    '        if (objstyle::ParseHexColor(m.formProps.count("WindowBgColor") ? m.formProps.at("WindowBgColor") : "", f)) {\n'
    '            o << "    ImGui::PushStyleColor(ImGuiCol_WindowBg, " << FVec4(f[0], f[1], f[2], f[3]) << ");\\n";\n'
    '            _nStyleCol++;\n'
    '        }\n'
    '        if (m.formProps.count("WindowRounding") && (float)atof(m.formProps.at("WindowRounding").c_str()) >= 0) {\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, " << FNum((float)atof(m.formProps.at("WindowRounding").c_str())) << "f);\\n";\n'
    '            _nStyleVar++;\n'
    '        }\n'
    '        if (m.formProps.count("WindowBorderSize") && (float)atof(m.formProps.at("WindowBorderSize").c_str()) >= 0) {\n'
    '            o << "    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, " << FNum((float)atof(m.formProps.at("WindowBorderSize").c_str())) << "f);\\n";\n'
    '            _nStyleVar++;\n'
    '        }\n'
    '    }\n'
    '    o << "    ImGui::SetNextWindowSize(ImVec2(" << m.formW << ", " << m.formH << "), ImGuiCond_FirstUseEver);\\n";\n'
    '    o << "    if (!ImGui::Begin(u8\\"" << EscCppUtf8(title) << "\\", nullptr, " << flags << ")) { ImGui::End();\\n";\n'
    '    if (_nStyleVar > 0) o << "        ImGui::PopStyleVar(" << _nStyleVar << ");\\n";\n'
    '    if (_nStyleCol > 0) o << "        ImGui::PopStyleColor(" << _nStyleCol << ");\\n";\n'
    '    o << "        return; }\\n";\n'
)
src = src[:i] + seg_final + src[j:]

# End() sonrası pop satırları
old_end = '    o << "    ImGui::End();\\n";\n    o << "}\\n";\n    o << "//== IMGOD:INIT-END'
new_end = ('    o << "    ImGui::End();\\n";\n'
           '    if (_nStyleVar > 0) o << "    ImGui::PopStyleVar(" << _nStyleVar << ");\\n";\n'
           '    if (_nStyleCol > 0) o << "    ImGui::PopStyleColor(" << _nStyleCol << ");\\n";\n'
           '    o << "}\\n";\n    o << "//== IMGOD:INIT-END')
assert old_end in src, "End anchor yok"
src = src.replace(old_end, new_end, 1)

# ---- 3) Global.h sarmalayıcı iyileştirmeleri ----
# GroupBox: Id alanı (b колlaps öncesi benzersiz child kimliği)
src = src.replace(
    'struct GroupBox {\n    std::string Text = "Grup";  ImVec2 Size = ImVec2(320, 160);\n    bool Begin() {\n        // WinForms GroupBox görünümü: çerçeveli child + üst kenarda başlık\n        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);\n        ImGui::BeginChild((Text + "_grp").c_str(), Size, ImGuiChildFlags_Borders);',
    'struct GroupBox {\n    std::string Text = "Grup";  std::string Id;  ImVec2 Size = ImVec2(320, 160);\n    bool Begin() {\n        // WinForms GroupBox görünümü: çerçeveli child + üst kenarda başlık\n        // Id boşsa Text+\"_grp\" — editör üretimi benzersiz Id bağlar\n        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);\n        ImGui::BeginChild(Id.empty() ? (Text + "_grp") : (Id + "_grp"), Size, ImGuiChildFlags_Borders);', 1)

# ModalPopup: "_pp" soneki yerine Text doğrudan (## sonekli Text benzersiz + temiz başlık)
src = src.replace(
    '    bool Begin() {\n        if (OpenReq && !ImGui::IsPopupOpen((Text + "_pp").c_str()))\n            ImGui::OpenPopup((Text + "_pp").c_str());\n        OpenReq = false;\n        return ImGui::BeginPopupModal((Text + "_pp").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);\n    }',
    '    bool Begin() {\n        if (OpenReq && !ImGui::IsPopupOpen(Text.c_str()))\n            ImGui::OpenPopup(Text.c_str());\n        OpenReq = false;\n        return ImGui::BeginPopupModal(Text.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);\n    }', 1)

# InputText: Password + ReadOnly bayrakları
src = src.replace(
    'struct InputText {\n    std::string Text = "Yazı";   std::string Value;  std::string Hint;\n    char Buf[512] = {};  bool InitBuf = false;  bool EnterPressed = false;\n    InputText() {}\n    bool Draw() {\n        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }\n        ImGuiInputTextFlags fl = 0;',
    'struct InputText {\n    std::string Text = "Yazı";   std::string Value;  std::string Hint;\n    bool Password = false;  bool ReadOnly = false;\n    char Buf[512] = {};  bool InitBuf = false;  bool EnterPressed = false;\n    InputText() {}\n    bool Draw() {\n        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }\n        ImGuiInputTextFlags fl = 0;\n        if (Password) fl |= ImGuiInputTextFlags_Password;\n        if (ReadOnly) fl |= ImGuiInputTextFlags_ReadOnly;', 1)
src = src.replace(
    'struct InputTextMultiline {\n    std::string Text = "Not";   std::string Value;  ImVec2 Size = ImVec2(280, 120);\n    char Buf[4096] = {};  bool InitBuf = false;\n    bool Draw() {\n        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }\n        bool ch = ImGui::InputTextMultiline(Text.c_str(), Buf, sizeof Buf, Size);',
    'struct InputTextMultiline {\n    std::string Text = "Not";   std::string Value;  bool ReadOnly = false;  ImVec2 Size = ImVec2(280, 120);\n    char Buf[4096] = {};  bool InitBuf = false;\n    bool Draw() {\n        if (!InitBuf) { snprintf(Buf, sizeof Buf, "%s", Value.c_str()); InitBuf = true; }\n        ImGuiInputTextFlags _fl = ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0;\n        bool ch = ImGui::InputTextMultiline(Text.c_str(), Buf, sizeof Buf, Size, _fl);', 1)

open(p, "w", encoding="utf-8").write(src)
print("CODEGEN PATCH OK")
