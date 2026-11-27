// splitter.h — FAZ 2.8: sürüklenebilir dikey AYRAÇ
// Bir sütunu ÜST / ALT iki panele bölen ince çubuk; fareyle sürükleyerek
// panellerin yüksekliğini kullanıcı ayarlar (RAD Studio / VS dock ayracı gibi).
#pragma once
#include "app.h"

namespace splitter {

// side: 0 = SOL sütun (üstte Kod Gezgini, altta Araç Kutusu)
//       1 = SAĞ sütun (üstte Formlar listesi, altta Özellikler/Olaylar)
HWND Create(HWND parent, int side);

// O sütunun üst panelinin güncel yüksekliği (layout buradan okur)
int  TopHeight(int side);
// Üst panel yüksekliğine YAZILABİLİR erişim (layout kıstırma uygulayıp geri yazar)
int& TopHeightRef(int side);

} // namespace splitter
