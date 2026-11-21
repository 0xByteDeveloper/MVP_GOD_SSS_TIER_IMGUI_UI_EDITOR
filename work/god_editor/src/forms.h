// ============================================================================
//  forms.h — çok formlu proje katmanı (park/unpark + Forms.info klasör formatı)
//  Özgün M2GodWin32 Faz 2.7 yapısının ImGui karşılığı:
//    Tek dosya : Proje.imgod
//    Klasör    : Forms.info + Form1.info + Form2.info ...
// ============================================================================
#pragma once
#include "model.h"
#include <vector>
#include <deque>

namespace forms {

struct FormEntry {
    std::string name;              // "Form1"
    Model model;                   // her formun kendi modeli + undo yığını
    bool parked = true;            // listede duruyor, tasarım yüzeyinde değil
};

void Init();                       // tek boş Form1 ile başlat
size_t Count();
int  ActiveIndex();
FormEntry* Active();
FormEntry* At(size_t i);
std::vector<FormEntry>& All();

// g_model ↔ aktif form senkronizasyonu (park/unpark)
void ParkActiveToModel();      // g_model → aktif form girdisi (kayıt/üretim öncesi çağır)

// form komutları
std::string NewForm(const std::string& baseName = "");   // yeni ad döner
void RenameActive(const std::string& newName);
bool DeleteAt(int index);
void SwitchTo(int index);          // aktif formu değiştir (aktif olana park/unpark)

// proje kaydet/yükle
bool SaveProjectAs(const std::string& path);       // .imgod tek dosya
bool LoadProject(const std::string& path);        // .imgod VEYA Forms.info algılar
bool SaveFolderProject(const std::string& dir);   // Forms.info + FormN.info
bool LoadFolderProject(const std::string& infoPath);

std::string ProjectDir();          // son kayıt klasörü
void SetProjectDir(const std::string& d);

// kod gezgini yardımcıları
std::vector<Ctrl*> ObjectsOf(int formIndex);
std::vector<std::pair<std::string, std::string>> FormEventsOf(int formIndex);

} // namespace forms
