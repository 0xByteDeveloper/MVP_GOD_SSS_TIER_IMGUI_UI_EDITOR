// ============================================================================
//  vsint.h — Visual Studio entegrasyonu (bağlanma katmanı)
//  Özgün M2GodWin32 vsint'in ImGui projesine uyarlanmış hali:
//    * Çalışan devenv.exe örnekleri Toolhelp32 anlık görüntüsüyle taranır
//      (yönetici uyumlu — pencere başlığından çözüm adı okunur).
//    * DTE ROT zenginleştirmesi bağlanınca best-effort denenir; yoksa
//      disk tabanlı çözüm taraması kullanılır (Win32 üzerinde çalışır,
//      Wine altında zarif bozulma: liste boş, VS'siz akış tam işler).
// ============================================================================
#pragma once
#include <string>
#include <vector>

namespace vsint {

struct VSInstance {
    unsigned long pid = 0;
    std::string version;       // "16.0" (VS2019) — pencere başlığından
    std::string title;         // ana pencere başlığı
    std::string solutionName;  // başlıktan ayıklanan çözüm adı
    std::string solutionPath;  // diskte .sln bulunduysa tam yol
};

// Çalışan VS örneklerini listele (Windows'ta Toolhelp32; başka ortamda boş döner)
std::vector<VSInstance> Enumerate();

// Bilinen köklerde (source\\repos vb.) .sln arar
std::string FindSlnOnDisk(const std::string& solutionName);
// Klasörde .sln tarar (bin/obj/.vs atlanır)
std::vector<std::string> FindSlnsUnder(const std::string& dir, int depth);

// Bağlanma durumu
bool Connect(const std::string& solutionPath, const std::string& projectDir);
void Disconnect();
bool IsConnected();
std::string ConnectedSln();
std::string ProjectDir();

// Çözümü Windows'ta aç (ShellExecute; Wine'da zarif başarısız)
bool OpenSolution(const std::string& slnPath);

} // namespace vsint
