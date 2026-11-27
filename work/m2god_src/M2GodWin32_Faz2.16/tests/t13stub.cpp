// t13stub.cpp — themetest13 için surface/paintctl bağlama koşulları
// (model+codegen zinciri surface'in PositionOne/SubclassForDesign'ini referans
// eder; test bunları çağırmaz — boş karşılıkları yeterli. paintctl gerçek
// bağlanır çünkü grafit derileri gerçeklenişin parçasıdır.)
#include "app.h"
#include "surface.h"
#include "paintctl.h"

namespace surface {
void OnModelDelete(Ctrl*) {}
void PositionOne(HWND, Ctrl*) {}
void SubclassForDesign(HWND) {}
void RecreateAll(HWND) {}
void CreateOnSurface(HWND, Ctrl*) {}
void SelectTabPage(HWND, Ctrl*, Ctrl*) {}
bool EscapeChain() { return false; }
}
