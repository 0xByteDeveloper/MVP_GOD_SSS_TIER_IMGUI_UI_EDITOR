#!/bin/bash
# check_template.sh — FAZ 2.12: global_template.inc'i çıplak Global.h olarak
# çıkarıp C++14 ile derle (şablon gövdesi üretilen uygulamanın Global.h'ıdır).
# Kullanım: bash tests/check_template.sh
set -e
cd "$(dirname "$0")/.."   # proje kökü
SRC=src/global_template.inc
OUT=tests/tmpl_global.h
# R"M2GODGLOBAL( ... )M2GODGLOBAL" gövdesini çıkar (ilk ve son satır düşer)
head -n -1 "$SRC" | tail -n +2 > "$OUT"
export PATH=/home/z/my-project/tools/llvm-mingw-20231114-ucrt-ubuntu-20.04-x86_64/bin:$PATH
TCROOT=/home/z/my-project/tools/llvm-mingw-20231114-ucrt-ubuntu-20.04-x86_64
MINC="$TCROOT/generic-w64-mingw32/include"
x86_64-w64-mingw32-g++ -std=c++14 -municode -O0 -Itests -Wall -Wno-unused-function \
  -isystem "$MINC/c++/v1" -isystem "$MINC" \
  tests/tmpl12_compile_check.cpp -o /tmp/tmpl12_check.exe \
  -static -static-libgcc -static-libstdc++ -lgdi32 -lcomctl32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi
echo "OK: global_template.inc (Global.h) C++14 ile derlendi"
