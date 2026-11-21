// formlist.h — FAZ 2.7: FORMLAR listesi paneli (sağ üst)
// RAD Studio / VS2019 .NET Solution Explorer tarzı: projedeki formlar listesi.
// Tek tık → form geçişi, çift tık → yeniden adlandır, sağ tık → menü.
#pragma once
#include "app.h"

namespace formlist {

// Paneli oluştur (ana pencere child'ı — sağ üst)
HWND Create(HWND parent);

// forms:: durumundan listeyi yeniden kur + aktif formu seç
void Refresh();

// Programatik seçim (ana pencereye bildirim YOK — echo döngüsü önlenir)
void Select(int index);

} // namespace formlist
