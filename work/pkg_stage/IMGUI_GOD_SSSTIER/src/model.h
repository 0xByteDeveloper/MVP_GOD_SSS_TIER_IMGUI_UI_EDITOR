// ============================================================================
//  model.h — tasarım modeli: Form + Ctrl (ImGui nesneleri) + undo/redo
//  Özgün M2GodWin32 modelinin ImGui evrenine birebir taşınmış hali.
// ============================================================================
#pragma once
#include "imgod.h"
#include "json.h"
#include "catalog.h"
#include <deque>

struct Ctrl {
    std::string name;              // "button1"
    std::string type;              // "Button"
    int x = 20, y = 20, w = 0, h = 0;   // parent-göreli koordinat (parent=null → form koordinatı)
    bool nonVisual = false;        // bileşen tepsisinde (Timer, FontRef)
    Ctrl* parent = nullptr;        // konteyner içi (ChildWindow/GroupBox/TabBar...)
    std::map<std::string, std::string> props;    // özellik → değer
    std::map<std::string, std::string> events;   // olay → handler ("button1_Click")

    std::string Get(const char* k, const char* def = "") const {
        auto it = props.find(k);
        return it == props.end() ? std::string(def) : it->second;
    }
    void Set(const char* k, const std::string& v) { props[k] = v; }
    bool GetB(const char* k, bool def = false) const {
        auto it = props.find(k);
        if (it == props.end()) return def;
        return it->second == "True" || it->second == "true" || it->second == "1";
    }
};

// ---------------------------------------------------------------------------
//  boyut yeteneği sabitleri — canvas/özellik paneli/kod üretimi ORTAK gerçeği
//  (kullanıcı hatası: combo/listbox vb. görsel boyutlanmıyordu, yalnız sayı değişiyordu)
enum {
    SIZECAP_AUTO = 0,   // içerikten ölçülür — tutamaç yok, özellik "(otomatik)"
    SIZECAP_W    = 1,   // yalnız genişlik ayarlanır (SetNextItemWidth)
    SIZECAP_FULL = 2    // hem genişlik hem yükseklik (ImVec2)
};
int SizeCapOf(const Ctrl* c);

class Model {
public:
    // Form (kök) özellikleri
    std::map<std::string, std::string> formProps;   // Text, Name, ClientSize, Flags, WindowBg...
    std::map<std::string, std::string> formEvents;  // Load → Form1_Load vb.
    std::vector<Ctrl*> ctrls;
    int formW = 430, formH = 460;

    // ---- çoklu seçim -------------------------------------------------------
    // selList: seçili nesneler (SON eleman = birincil seçim). Boşsa form seçilidir.
    std::vector<Ctrl*> selList;
    Ctrl* selected = nullptr;        // birincil seçim (selList.back() ile senkron)
    bool formSelected = true;

    bool  IsSelected(const Ctrl* c) const;
    Ctrl* Primary() const { return selList.empty() ? nullptr : selList.back(); }
    std::vector<Ctrl*> ValidSelection() const;          // var olan kontroller filtreli
    void  SelectOnly(Ctrl* c);                            // tek seçim (c=null → form)
    void  SetSelection(const std::vector<Ctrl*>& list);  // toplu seçim (rubber band)
    void  ToggleSel(Ctrl* c);                             // Shift/Ctrl tık
    void  AddSel(Ctrl* c);                                // seçime ekle
    void  SelectForm();                                  // form seçimi (seçimi temizle)
    void  SelectAll();                                    // Ctrl+A
    void  SyncSelection();                                // silinen/adı değişen sonrası düzelt

    // ---- pano (Ctrl+C/X/V — M2God davranışı) ------------------------------
    // Kopyalanan nesneler JSON olarak tutulur; yapıştırılınca adlar TİP
    // sayacıyla yeniden üretilir (denemebutton → button3 gibi).
    std::string clipboardJson;                            // nesne JSON dizisi
    bool  CopySelection();                                 // seçili + alt kontrolleri JSON'a al
    bool  CutSelection();                                  // kopyala + sil
    int   PasteClipboard(int dx, int dy);                  // yeni adlarla ekle (dönen: eklenen sayı)
    int   pasteCount = 0;                                  // yapıştırma kademeli ofseti

    // --- parent-child yardımcıları --------------------------------------
    std::vector<Ctrl*> ChildrenOf(Ctrl* p) const;
    bool IsAncestorOf(Ctrl* a, Ctrl* c) const;
    void Reparent(Ctrl* c, Ctrl* newParent, int px, int py);

    // --- undo / redo -----------------------------------------------------
    void PushUndo();
    bool Undo();
    bool Redo();
    bool CanUndo() const { return !undo.empty(); }
    bool CanRedo() const { return !redo.empty(); }
    std::deque<std::string> DetachUndo();
    std::deque<std::string> DetachRedo();
    void AttachUndo(std::deque<std::string> u, std::deque<std::string> r);
    void ClearUndo();

    // --- kontroller --------------------------------------------------------
    std::string UniqueName(const std::string& type);
    Ctrl* Add(const std::string& type, int x, int y, Ctrl* parent = nullptr);
    void Delete(Ctrl* c);                 // alt kontrollerle birlikte
    void DeleteList(const std::vector<Ctrl*>& list);   // çoklu silme (Ctrl+X/Del)
    Ctrl* Clone(Ctrl* src, int dx, int dy);
    Ctrl* FindByName(const std::string& name);
    void Rename(Ctrl* c, const std::string& newName);
    void BringToFront(Ctrl* c);
    void SendToBack(Ctrl* c);
    void ClearAll();
    std::string DefaultEventOf(const Ctrl* c) const;

    // --- kaydet / yükle (.imgod JSON — özgün v3 şemasının ImGui karşılığı) --
    json::Value ToJson() const;
    bool FromJson(const json::Value& v);
    bool Save(const std::string& path);
    bool Load(const std::string& path);

private:
    std::deque<std::string> undo, redo;
    std::map<std::string, int> nameCounters;
    std::string Snapshot() const;        // tüm modelin JSON'u
    void Restore(const std::string& js);
    // pano içi derin klon yardımımcısı
    Ctrl* PasteCtrlFromJson(const json::Value& v, const std::map<std::string, std::string>& nameMap,
                             Ctrl* parentOverride, int dx, int dy);
};

extern Model g_model;
