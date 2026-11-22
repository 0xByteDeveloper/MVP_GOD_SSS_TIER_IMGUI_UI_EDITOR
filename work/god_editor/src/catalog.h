// ============================================================================
//  catalog.h — ImGui nesne kataloğu (Araç Kutusu + Özellikler + Olaylar)
//  Özgün M2GodWin32'nin Config_info.json (.NET WinForms kataloğu) yerine
//  IMGUI widget evreni: her nesne tipi gerçek ImGui API çağrısına karşılık
//  gelir ve WYSIWYG canvas'ta AYNEN böyle render edilir.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// ---------------------------------------------------------------------------
//  Özellik tanımı
struct PropDef {
    std::string name;        // "Text", "Min", "Items"...
    std::string type;        // "string" | "bool" | "int" | "float" | "color" | "enum" | "items" | "vec2"
    std::string category;    // "Görünüm", "Davranış", "Yerleşim", "Veri"
    std::string def;         // varsayılan (string formu)
    std::string enumType;    // enum ise "ImGuiSliderFlags" gibi
    std::string hint;        // panel ipucu (boş olabilir)
};
struct EventDef {
    std::string name;        // "Click" / "ValueChanged"...
    std::string desc;        // "Tıklandığında"
};
struct ElementDef {
    std::string name;        // "Button" — ImGui dünyasındaki görünen ad
    std::string cppApi;      // "ImGui::Button" — kod üretiminde kullanılan API
    std::string kind;        // "control" | "container" | "component" | "menu"
    std::string catTr;       // Araç kutusu kategorisi: "Ortak", "Konteynerler", "Veri & Grafik", "Bileşenler", "Menü"
    std::string defaultEvent;
    bool toolboxVisible = true;
    int  defW = 0, defH = 0; // varsayılan tasarım boyutu (0 = ImGui kendi ölçüsü)
    std::vector<PropDef> props;
    std::vector<EventDef> events;

    const PropDef* FindProp(const std::string& n) const {
        for (auto& p : props) if (p.name == n) return &p;
        return nullptr;
    }
    bool HasEvent(const std::string& n) const {
        for (auto& e : events) if (e.name == n) return true;
        return false;
    }
};

class Catalog {
public:
    std::vector<std::string> categories;                 // araç kutusu kategori sırası
    std::map<std::string, std::vector<std::string>> catElements;
    std::map<std::string, ElementDef> elements;           // name → def

    void LoadBuiltin();                                   // derlemeye gömülü ImGui kataloğu
    const ElementDef* Find(const std::string& name) const {
        auto it = elements.find(name);
        return it == elements.end() ? nullptr : &it->second;
    }
    size_t ElementCount() const { return elements.size(); }
};
extern Catalog g_catalog;

// Katalog yardımcıları
bool IsContainerType(const std::string& type);
std::string HandlerOf(const std::string& objName, const std::string& ev);

// 6. TUR aile yardımcıları (canvas + özellik paneli + kod üretimi TEK gerçek)
// Yan-metinli (etiketi DENETİMİN DIŞINDA çizilen) tipler — "Etiketleri Gizle"
// ayarı bu aileyi etkiler: gizliyken etiket yalnız "##ad" kimliğine iner.
bool HasAdjacentLabel(const std::string& type);
// Çerçeve-yüksekliği ölçeklenebilen tek-satır giriş ailesi (TextBox dahil):
// boyut (W+H) — yükseklik FramePadding ölçeklemesiyle çerçeve satırını büyütür.
// canvas VE üretilen IGOD sarmalayıcıları AYNI dönüşümü uygular (WYSIWYG).
bool IsFrameSizedType(const std::string& type);
// Sürükle-bırak ile İÇİNE nesne bırakılabilen konteynerler (reparent hedefleri).
bool IsDropTargetContainer(const std::string& type);
