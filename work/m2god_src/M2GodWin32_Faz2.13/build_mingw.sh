#!/bin/bash
# M2GodWin32 — mingw-w64 derleme betiği (Linux cross / portatif)
# FAZ 2.13: TEMA ÇÖKMESİ KÖKTEN ÇÖZÜLDÜ (2 tema değişiminde çökme):
# aktivasyon bağlamı + panel imha/yeniden yaratma döngüsü KALDIRILDI;
# tema geçişi paintctl ile YERİNDE deri değişimidir (hiçbir pencere imha edilmez).
# APPLE & MAC gerçek AQUA oldu (kapsül buton, mac alan kenarı); MODERN ayrıntılı
# WinForms görünümü; RAD STUDIO TEMASI SİLİNDİ (kullanıcı isteği).
# Çıktı: Bin/M2GodWin32_Faz2.13.exe
set -e
cd "$(dirname "$0")"   # proje klasörü (build/ ve src/ burada)
export PATH=/home/z/my-project/tools/llvm-mingw-20231114-ucrt-ubuntu-20.04-x86_64/bin:$PATH
# llvm-mingw başlıkları generic-w64-mingw32 altında; sürücü otomatik bulamazsa
# açık yol ver (her iki durumda da zararsız)
TCROOT=/home/z/my-project/tools/llvm-mingw-20231114-ucrt-ubuntu-20.04-x86_64
MINC="$TCROOT/generic-w64-mingw32/include"
MLIB="$TCROOT/x86_64-w64-mingw32/lib"
INCFLAGS="-isystem $MINC/c++/v1 -isystem $MINC"
LIBFLAGS="-L $MLIB"
mkdir -p Bin obj
echo "[1/3] Kaynak (.rc) derleniyor..."
x86_64-w64-mingw32-windres app.rc -O coff -o obj/app.res
echo "[2/3] C++ kaynakları derleniyor..."
SRCS="src/main.cpp src/utils.cpp src/json.cpp src/catalog.cpp src/model.cpp src/factory.cpp src/surface.cpp src/toolbox.cpp src/props.cpp src/codegen.cpp src/vsint.cpp src/mainwnd.cpp src/forms.cpp src/formlist.cpp src/splitter.cpp src/codetree.cpp src/theme.cpp src/paintctl.cpp"
FLAGS="-std=c++17 -municode -O2 -Isrc -Wall -Wno-unused-function $INCFLAGS"
OBJS=""
for f in $SRCS; do
  o="obj/$(basename "$f" .cpp).o"
  x86_64-w64-mingw32-g++ $FLAGS -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
echo "[3/3] Bağlanıyor..."
x86_64-w64-mingw32-g++ $OBJS obj/app.res -o Bin/M2GodWin32_Faz2.13.exe \
  -municode -mwindows -static -static-libgcc -static-libstdc++ $LIBFLAGS \
  -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lshlwapi -lole32 -loleaut32 -luuid -ladvapi32
echo "OK: Bin/M2GodWin32_Faz2.13.exe"
