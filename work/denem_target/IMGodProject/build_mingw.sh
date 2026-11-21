#!/bin/bash
# IMGUI_GOD_SSSTIER üretimi — MinGW/GCC/clang bağımsız derleme (x64)
set -e
cd "$(dirname "$0")"
mkdir -p Bin
CXX=${CXX:-}
if [ -z "$CXX" ]; then
  for c in x86_64-w64-mingw32-clang++ x86_64-w64-mingw32-g++-posix x86_64-w64-mingw32-g++ i686-w64-mingw32-g++; do
    command -v "$c" >/dev/null 2>&1 && CXX="$c" && break
  done
fi
[ -n "$CXX" ] || { echo 'MinGW derleyicisi bulunamadı (x86_64-w64-mingw32-g++)'; exit 1; }
SRCS="main.cpp Form1.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_dx11.cpp imgui/backends/imgui_impl_win32.cpp"
"$CXX" -O2 -std=c++17 -municode -mwindows -static -o Bin/IMGodProject.exe $SRCS  -I. -Iimgui -Iimgui/backends -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -luuid -lole32 -lgdi32 -lwinmm -limm32
echo OK: Bin/IMGodProject.exe
