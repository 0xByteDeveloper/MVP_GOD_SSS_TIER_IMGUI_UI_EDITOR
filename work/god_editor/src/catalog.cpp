// ============================================================================
//  catalog.cpp — ImGui nesne kataloğu: 24 araç kutusu nesnesi, TÜM özellikler
//  (görünüm + veri + yerleşim + RENK + STİL) ve olaylar. Her nesne WYSIWYG
//  canvas'ta ve üretilen kodda AYNEN ImGui API'siyle render edilir.
//
//  RENK özellikleri ("Renk" kategorisi): "#RRGGBB" değerli; boş = kalıtım.
//  STİL özellikleri: FrameRounding (-1 = kalıtım), FrameBorderSize (-1),
//  Alpha (1 = tam), Disabled, Repeat (düğme), Password (giriş).
//  Objstyle (objstyle.cpp) bunları PushStyleColor/PushStyleVar'a çevirir;
//  canvas ve üretilen .cpp AYNI diziyi kullanır (birebir WYSIWYG).
// ============================================================================
#include "catalog.h"

Catalog g_catalog;

bool IsContainerType(const std::string& type) {
    return type == "Panel" || type == "ChildWindow" || type == "GroupBox" || type == "CollapsingHeader" ||
           type == "TabBar" || type == "TreeNode" || type == "Table" || type == "Menu" ||
           type == "Popup";
}
std::string HandlerOf(const std::string& objName, const std::string& ev) {
    return objName + "_" + ev;
}

// yardımcı: özellik üreticileri (kısa tutmak için)
static PropDef P(const char* n, const char* ty, const char* cat, const char* def, const char* en = "", const char* hint = "") {
    PropDef p; p.name = n; p.type = ty; p.category = cat; p.def = def; p.enumType = en; p.hint = hint; return p;
}
static EventDef E(const char* n, const char* d) { EventDef e; e.name = n; e.desc = d; return e; }

// ortak renk/stil özellik paketleri
static void AddStyleProps(std::vector<PropDef>& props) {
    props.push_back(P("FrameRounding","float","Stil","-1","","Köşe yuvarlama px (-1 = tema kalıtımı)"));
    props.push_back(P("FrameBorderSize","float","Stil","-1","","Çerçeve kalınlığı px (-1 = tema kalıtımı)"));
    props.push_back(P("Alpha","float","Stil","1","","Saydamlık 0..1 (1 = tam opak)"));
    props.push_back(P("Disabled","bool","Stil","False","","Devre dışı (soluk + tıklanamaz)"));
}
static void AddTextColors(std::vector<PropDef>& props) {
    props.push_back(P("TextColor","color","Renk","","","Metin rengi (boş = tema kalıtımı)"));
}

void Catalog::LoadBuiltin() {
    categories = { "Ortak", "Konteynerler", "Veri & Grafik", "Bileşenler", "Menü" };
    elements.clear();
    catElements.clear();

    // ---- ORTAK ---------------------------------------------------------------
    { ElementDef d; d.name = "Button"; d.cppApi = "ImGui::Button"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "Click"; d.defW = 120; d.defH = 28;
      d.props = { P("Text","string","Görünüm","Buton"), P("SizeArgW","int","Yerleşim","120",""), P("SizeArgH","int","Yerleşim","28",""),
                  P("Small","bool","Görünüm","False","","Küçük düğme (SmallButton)"),
                  P("Repeat","bool","Davranış","False","","Basılı tutunca tekrar tetikler"),
                  P("Tooltip","string","Görünüm","","","Fare üstündeyken ipucu") };
      AddTextColors(d.props);
      d.props.push_back(P("ButtonColor","color","Renk","","","Düğme zemini"));
      d.props.push_back(P("ButtonHoverColor","color","Renk","","","Fare üstündeyken zemin"));
      d.props.push_back(P("ButtonActiveColor","color","Renk","","","Basılıyken zemin"));
      AddStyleProps(d.props);
      d.events = { E("Click","Tıklandığında"), E("Hovered","Fare üstüne gelince"), E("Held","Basılı tutulunca") };
      elements["Button"] = d; }
    { ElementDef d; d.name = "Checkbox"; d.cppApi = "ImGui::Checkbox"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "Checked"; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Seçenek"), P("Checked","bool","Veri","False") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Onay kutusu zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      d.props.push_back(P("CheckMarkColor","color","Renk","","","İşaret rengi"));
      AddStyleProps(d.props);
      d.events = { E("Checked","İşaret değişince") };
      elements["Checkbox"] = d; }
    { ElementDef d; d.name = "RadioButton"; d.cppApi = "ImGui::RadioButton"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "Click"; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Seçim"), P("Active","bool","Veri","False") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Radyo zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      d.props.push_back(P("CheckMarkColor","color","Renk","","","Nokta rengi"));
      AddStyleProps(d.props);
      d.events = { E("Click","Seçilince") };
      elements["RadioButton"] = d; }
    { ElementDef d; d.name = "SliderFloat"; d.cppApi = "ImGui::SliderFloat"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 200; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Değer"), P("Min","float","Veri","0"), P("Max","float","Veri","100"),
                  P("Value","float","Veri","0"), P("Format","string","Veri","%.1f") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Kaydırma çubuğu zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      d.props.push_back(P("SliderGrabColor","color","Renk","","","Tutamaç rengi"));
      d.props.push_back(P("SliderGrabActiveColor","color","Renk","","","Aktif tutamaç"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer kayarken"), E("Deactivated","Kaydırma bitince") };
      elements["SliderFloat"] = d; }
    { ElementDef d; d.name = "SliderInt"; d.cppApi = "ImGui::SliderInt"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 200; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Değer"), P("Min","int","Veri","0"), P("Max","int","Veri","100"),
                  P("Value","int","Veri","0"), P("Format","string","Veri","%d") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Kaydırma çubuğu zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      d.props.push_back(P("SliderGrabColor","color","Renk","","","Tutamaç rengi"));
      d.props.push_back(P("SliderGrabActiveColor","color","Renk","","","Aktif tutamaç"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer kayarken") };
      elements["SliderInt"] = d; }
    { ElementDef d; d.name = "DragFloat"; d.cppApi = "ImGui::DragFloat"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 180; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Değer"), P("Min","float","Veri","0"), P("Max","float","Veri","100"),
                  P("Value","float","Veri","0"), P("Speed","float","Veri","1") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer sürüklenince") };
      elements["DragFloat"] = d; }
    { ElementDef d; d.name = "DragInt"; d.cppApi = "ImGui::DragInt"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 180; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Değer"), P("Min","int","Veri","0"), P("Max","int","Veri","100"),
                  P("Value","int","Veri","0"), P("Speed","float","Veri","1") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Basılıyken zemin"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer sürüklenince") };
      elements["DragInt"] = d; }
    { ElementDef d; d.name = "InputText"; d.cppApi = "ImGui::InputText"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "TextChanged"; d.defW = 200; d.defH = 0; d.toolboxVisible = false;   // araç kutusunda "TextBox" adıyla (M2God sözlüğü)
      d.props = { P("Text","string","Görünüm","Yazı"), P("Value","string","Veri",""),
                  P("Hint","string","Görünüm","","","Boşken soluk ipucu"),
                  P("Password","bool","Davranış","False","","Yıldız maskesi"),
                  P("ReadOnly","bool","Davranış","False","","Salt okunur"),
                  P("Width","int","Yerleşim","200") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("TextChanged","Yazı değişince"), E("EnterPressed","Enter'a basılınca") };
      elements["InputText"] = d; }
    { ElementDef d; d.name = "TextBox"; d.cppApi = "ImGui::InputText"; d.kind = "control"; d.catTr = "Ortak";
      // KULLANICI TALEBİ: "textbox yok" — M2God/WinForms sözlüğüyle araç kutusunda
      // TextBox (tek satır metin girişi; kod üretiminde ImGui::InputText).
      d.defaultEvent = "TextChanged"; d.defW = 200; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Yazı"), P("Value","string","Veri",""),
                  P("Hint","string","Görünüm","","","Boşken soluk ipucu"),
                  P("Password","bool","Davranış","False","","Yıldız maskesi"),
                  P("ReadOnly","bool","Davranış","False","","Salt okunur"),
                  P("Width","int","Yerleşim","200") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("TextChanged","Yazı değişince"), E("EnterPressed","Enter'a basılınca") };
      elements["TextBox"] = d; }
    { ElementDef d; d.name = "InputTextMultiline"; d.cppApi = "ImGui::InputTextMultiline"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "TextChanged"; d.defW = 280; d.defH = 120; d.toolboxVisible = false;  // araç kutusunda "TextBoxMultiline"
      d.props = { P("Text","string","Görünüm","Not"), P("Value","string","Veri",""),
                  P("ReadOnly","bool","Davranış","False","","Salt okunur"),
                  P("Width","int","Yerleşim","280"), P("Height","int","Yerleşim","120") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("TextChanged","Yazı değişince") };
      elements["InputTextMultiline"] = d; }
    { ElementDef d; d.name = "TextBoxMultiline"; d.cppApi = "ImGui::InputTextMultiline"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "TextChanged"; d.defW = 280; d.defH = 120;
      d.props = { P("Text","string","Görünüm","Not"), P("Value","string","Veri",""),
                  P("ReadOnly","bool","Davranış","False","","Salt okunur"),
                  P("Width","int","Yerleşim","280"), P("Height","int","Yerleşim","120") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("TextChanged","Yazı değişince") };
      elements["TextBoxMultiline"] = d; }
    { ElementDef d; d.name = "InputInt"; d.cppApi = "ImGui::InputInt"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 160; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Sayı"), P("Value","int","Veri","0"), P("Step","int","Veri","1") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer değişince") };
      elements["InputInt"] = d; }
    { ElementDef d; d.name = "InputFloat"; d.cppApi = "ImGui::InputFloat"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ValueChanged"; d.defW = 160; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Sayı"), P("Value","float","Veri","0"), P("Step","float","Veri","1") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Giriş zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Odaktayken zemin"));
      AddStyleProps(d.props);
      d.events = { E("ValueChanged","Değer değişince") };
      elements["InputFloat"] = d; }
    { ElementDef d; d.name = "Combo"; d.cppApi = "ImGui::Combo"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "SelectionChanged"; d.defW = 200; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Seçim"), P("Items","items","Veri","Seçenek 1|Seçenek 2|Seçenek 3","","Öğeler | ile ayrılır"),
                  P("Selected","int","Veri","0"), P("Width","int","Yerleşim","200") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Kutu zemini"));
      d.props.push_back(P("FrameHoverColor","color","Renk","","","Üzerindeyken zemin"));
      d.props.push_back(P("FrameActiveColor","color","Renk","","","Açıkken zemin"));
      AddStyleProps(d.props);
      d.events = { E("SelectionChanged","Seçim değişince") };
      elements["Combo"] = d; }
    { ElementDef d; d.name = "ListBox"; d.cppApi = "ImGui::ListBox"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "SelectionChanged"; d.defW = 200; d.defH = 120;
      d.props = { P("Text","string","Görünüm","Liste"), P("Items","items","Veri","Öğe 1|Öğe 2|Öğe 3","","Öğeler | ile ayrılır"),
                  P("Selected","int","Veri","0"), P("HeightItems","int","Yerleşim","5") };
      AddTextColors(d.props);
      d.props.push_back(P("FrameBgColor","color","Renk","","","Liste zemini"));
      AddStyleProps(d.props);
      d.events = { E("SelectionChanged","Seçim değişince") };
      elements["ListBox"] = d; }
    { ElementDef d; d.name = "ColorEdit4"; d.cppApi = "ImGui::ColorEdit4"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = "ColorChanged"; d.defW = 180; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Renk"), P("R","int","Veri","255"), P("G","int","Veri","255"),
                  P("B","int","Veri","255"), P("A","int","Veri","255"), P("AlphaPreview","bool","Görünüm","False","","Alfa kanalını da düzenle") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = { E("ColorChanged","Renk değişince") };
      elements["ColorEdit4"] = d; }
    { ElementDef d; d.name = "ProgressBar"; d.cppApi = "ImGui::ProgressBar"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 200; d.defH = 18;
      d.props = { P("Value","float","Veri","0.5"), P("Overlay","string","Görünüm",""),
                  P("Width","int","Yerleşim","200"), P("Height","int","Yerleşim","18") };
      AddTextColors(d.props);
      d.props.push_back(P("ProgressColor","color","Renk","","","Dolgu rengi"));
      AddStyleProps(d.props);
      d.events = { E("Hovered","Fare üstüne gelince") };
      elements["ProgressBar"] = d; }
    { ElementDef d; d.name = "Separator"; d.cppApi = "IGOD::Separator"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 200; d.defH = 2;
      // KULLANICI TALEBİ: "seperator ince çubuk gibi, büyütüp küçültemiyorum" —
      // artık GERÇEK boyutlandırılabilir ayraç (W×H dolgu çubuğu).
      d.props = { P("Width","int","Yerleşim","200","","Genişlik (px)"),
                  P("Height","int","Yerleşim","2","","Kalınlık (px)") };
      d.props.push_back(P("SeparatorColor","color","Renk","","","Çubuk rengi"));
      AddStyleProps(d.props);
      d.events = {};
      elements["Separator"] = d; }
    { ElementDef d; d.name = "SeparatorText"; d.cppApi = "IGOD::SeparatorText"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Bölüm"), P("Width","int","Yerleşim","0","","0 = mevcut genişlik") };
      AddTextColors(d.props);
      d.props.push_back(P("SeparatorColor","color","Renk","","","Çizgi rengi"));
      AddStyleProps(d.props);
      d.events = {};
      elements["SeparatorText"] = d; }
    { ElementDef d; d.name = "Text"; d.cppApi = "ImGui::Text"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Etiket"), P("Wrapped","bool","Görünüm","False","","Satır sonunda kaydır"),
                  P("Width","int","Yerleşim","0","","0 = içerik genişliği; >0 = bu genişlikte kaydır") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = {};
      elements["Text"] = d; }
    { ElementDef d; d.name = "TextColored"; d.cppApi = "ImGui::TextColored"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Renkli etiket"), P("R","int","Veri","255"), P("G","int","Veri","200"), P("B","int","Veri","80"),
                  P("Width","int","Yerleşim","0","","0 = içerik genişliği; >0 = bu genişlikte kaydır") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = {};
      elements["TextColored"] = d; }
    { ElementDef d; d.name = "BulletText"; d.cppApi = "IGOD::BulletText"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Madde"),
                  P("Width","int","Yerleşim","0","","0 = içerik genişliği; >0 = bu genişliğe sığdır (kısalt)") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = {};
      elements["BulletText"] = d; }
    { ElementDef d; d.name = "LabelText"; d.cppApi = "ImGui::LabelText"; d.kind = "control"; d.catTr = "Ortak";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Alan"), P("Value","string","Veri","değer"),
                  P("Width","int","Yerleşim","0","","Değer bu genişliğin sağ ucunda hizalanır") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = {};
      elements["LabelText"] = d; }

    // ---- KONTEYNERLER --------------------------------------------------------
    { ElementDef d; d.name = "Panel"; d.cppApi = "ImGui::BeginChild"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 300; d.defH = 200;
      d.props = { P("Text","string","Görünüm","Panel","","Kimlik için (görünür başlık çizilmez)"),
                  P("Border","bool","Görünüm","True","","Kenarlık çizgisi"),
                  P("Width","int","Yerleşim","300"), P("Height","int","Yerleşim","200"),
                  P("Rounding","float","Görünüm","-1","","Köşe yuvarlama px (-1 = tema)") };
      AddTextColors(d.props);
      d.props.push_back(P("BgColor","color","Renk","","","Panel zemini"));
      d.props.push_back(P("BorderColor","color","Renk","","","Kenarlık"));
      AddStyleProps(d.props);
      d.events = { E("Hovered","Fare üstüne gelince") };
      elements["Panel"] = d; }
    { ElementDef d; d.name = "ChildWindow"; d.cppApi = "ImGui::BeginChild"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 300; d.defH = 200;
      d.props = { P("Text","string","Görünüm","Child1"), P("Border","bool","Görünüm","True"),
                  P("Width","int","Yerleşim","300"), P("Height","int","Yerleşim","200") };
      AddTextColors(d.props);
      d.props.push_back(P("BgColor","color","Renk","","","Child zemini"));
      d.props.push_back(P("BorderColor","color","Renk","","","Kenarlık"));
      AddStyleProps(d.props);
      d.events = { E("Hovered","Fare üstüne gelince") };
      elements["ChildWindow"] = d; }
    { ElementDef d; d.name = "GroupBox"; d.cppApi = "IGOD::GroupBox"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 320; d.defH = 160;
      d.props = { P("Text","string","Görünüm","Grup"), P("Width","int","Yerleşim","320"), P("Height","int","Yerleşim","160") };
      AddTextColors(d.props);
      d.props.push_back(P("BgColor","color","Renk","","","Grup zemini"));
      d.props.push_back(P("BorderColor","color","Renk","","","Çerçeve"));
      AddStyleProps(d.props);
      d.events = {};
      elements["GroupBox"] = d; }
    { ElementDef d; d.name = "CollapsingHeader"; d.cppApi = "ImGui::CollapsingHeader"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Başlık"), P("DefaultOpen","bool","Davranış","True"),
                  P("Width","int","Yerleşim","0","","0 = mevcut genişlik; >0 = bu genişlikte sınırla") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = { E("Toggled","Aç/kapa değişince") };
      elements["CollapsingHeader"] = d; }
    { ElementDef d; d.name = "TabBar"; d.cppApi = "ImGui::BeginTabBar"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 400; d.defH = 220;
      // KULLANICI TALEBİ: "tabbar boyutunu küçültüp büyültemiyorum" — W×H artık
      // tasarım boyutu (child bölge sarmalamasıyla).
      d.props = { P("Text","string","Görünüm","Sekmeler","","Kimlik için"), P("Items","items","Veri","Genel|Ayarlar|Hakkında","","Sekme başlıkları | ile"),
                  P("Width","int","Yerleşim","400","","Bölge genişliği"), P("Height","int","Yerleşim","220","","Bölge yüksekliği") };
      AddTextColors(d.props);
      d.props.push_back(P("TabSelectedColor","color","Renk","","","Seçili sekme zemini"));
      AddStyleProps(d.props);
      d.events = { E("TabChanged","Aktif sekme değişince") };
      elements["TabBar"] = d; }
    { ElementDef d; d.name = "TreeNode"; d.cppApi = "ImGui::TreeNode"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Text","string","Görünüm","Düğüm"), P("DefaultOpen","bool","Davranış","False"),
                  P("Width","int","Yerleşim","0","","0 = mevcut genişlik; >0 = bu genişlikte sınırla") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = { E("Toggled","Aç/kapa değişince") };
      elements["TreeNode"] = d; }
    { ElementDef d; d.name = "Popup"; d.cppApi = "ImGui::BeginPopupModal"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 320; d.defH = 140;
      d.props = { P("Text","string","Görünüm","Pencere") };
      AddTextColors(d.props);
      AddStyleProps(d.props);
      d.events = { E("Opened","Açılınca"), E("Closed","Kapanınca") };
      elements["Popup"] = d; }
    { ElementDef d; d.name = "Table"; d.cppApi = "ImGui::BeginTable"; d.kind = "container"; d.catTr = "Konteynerler";
      d.defaultEvent = ""; d.defW = 380; d.defH = 120;
      d.props = { P("Text","string","Görünüm","Tablo"), P("Columns","int","Veri","3"),
                  P("Items","items","Veri","Sütun1|Sütun2|Sütun3","","Başlık hücreleri | ile"), P("Rows","int","Veri","3") };
      AddTextColors(d.props);
      d.props.push_back(P("HeaderBgColor","color","Renk","","","Başlık satırı zemini"));
      AddStyleProps(d.props);
      d.events = {};
      elements["Table"] = d; }

    // ---- VERİ & GRAFİK ------------------------------------------------------
    { ElementDef d; d.name = "PlotLines"; d.cppApi = "ImGui::PlotLines"; d.kind = "control"; d.catTr = "Veri & Grafik";
      d.defaultEvent = ""; d.defW = 280; d.defH = 120;
      d.props = { P("Text","string","Görünüm","Grafik"), P("Items","items","Veri","0.1|0.4|0.2|0.9|0.5","","Veriler | ile"),
                  P("ScaleMin","float","Veri","0"), P("ScaleMax","float","Veri","1"),
                  P("Width","int","Yerleşim","280"), P("Height","int","Yerleşim","120") };
      AddTextColors(d.props);
      d.props.push_back(P("PlotColor","color","Renk","","","Çizgi rengi"));
      AddStyleProps(d.props);
      d.events = { E("Hovered","Fare üstüne gelince") };
      elements["PlotLines"] = d; }
    { ElementDef d; d.name = "PlotHistogram"; d.cppApi = "ImGui::PlotHistogram"; d.kind = "control"; d.catTr = "Veri & Grafik";
      d.defaultEvent = ""; d.defW = 280; d.defH = 120;
      d.props = { P("Text","string","Görünüm","Histogram"), P("Items","items","Veri","0.2|0.6|0.3",""),
                  P("Width","int","Yerleşim","280"), P("Height","int","Yerleşim","120") };
      AddTextColors(d.props);
      d.props.push_back(P("PlotColor","color","Renk","","","Bar rengi"));
      AddStyleProps(d.props);
      d.events = {};
      elements["PlotHistogram"] = d; }

    // ---- BİLEŞENLER (görsel olmayan) ---------------------------------------
    { ElementDef d; d.name = "Timer"; d.cppApi = "IGOD::Timer"; d.kind = "component"; d.catTr = "Bileşenler";
      d.defaultEvent = "Tick"; d.defW = 0; d.defH = 0; d.toolboxVisible = true;
      d.props = { P("Interval","int","Davranış","1000","","Milisaniye"), P("Enabled","bool","Davranış","False") };
      d.events = { E("Tick","Her aralıkta") };
      elements["Timer"] = d; }
    { ElementDef d; d.name = "FontRef"; d.cppApi = "IGOD::FontRef"; d.kind = "component"; d.catTr = "Bileşenler";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0; d.toolboxVisible = true;
      d.props = { P("Text","string","Görünüm","Arial"), P("Size","int","Görünüm","18"), P("Bold","bool","Görünüm","False") };
      d.events = {};
      elements["FontRef"] = d; }

    // ---- MENÜ ----------------------------------------------------------------
    { ElementDef d; d.name = "Menu"; d.cppApi = "IGOD::MenuBar"; d.kind = "menu"; d.catTr = "Menü";
      d.defaultEvent = ""; d.defW = 0; d.defH = 0;
      d.props = { P("Items","items","Veri","Dosya|Düzen|Yardım","","Menü başlıkları | ile"),
                  P("Commands","items","Veri","Dosya: Çıkış","","Menü: Komut çiftleri | ile") };
      d.events = { E("MenuClick","Menü komutuna tıklanınca") };
      elements["Menu"] = d; }

    // Araç kutusu kategori haritası
    for (auto& kv : elements) {
        if (!kv.second.toolboxVisible) continue;
        catElements[kv.second.catTr].push_back(kv.first);
    }
    // kategori içinde alfabetik (Menü en sonda kalır, sıralama kategori listesiyle)
    for (auto& kv : catElements)
        std::sort(kv.second.begin(), kv.second.end());
}
