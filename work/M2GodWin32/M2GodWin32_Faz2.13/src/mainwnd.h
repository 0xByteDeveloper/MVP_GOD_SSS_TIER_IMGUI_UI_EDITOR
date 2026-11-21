// mainwnd.h — ana çerçeve penceresi
#pragma once
#include "app.h"
namespace mainwnd {
int Run(HINSTANCE hInst, LPWSTR cmdLine);   // mesaj döngüsü

// FAZ 2.8: ayraç (splitter) sürüklenirken layout'u yeniden dizer
// (splitter.cpp çağırır; dışa kapalı tutmak için sarmalayıcı)
void RefreshLayout();
}
