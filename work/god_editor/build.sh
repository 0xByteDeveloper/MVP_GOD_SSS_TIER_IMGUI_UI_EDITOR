#!/bin/bash
# IMGUI_GOD_SSSTIER editör derlemesi (llvm-mingw — x64/x86)
set -e
cd "$(dirname "$0")"
L=${LLVMMINGW:-/home/z/tools/llvm-mingw/bin}
IMGUI_DIR=${IMGUI_DIR:-./imgui}
ARCH=${1:-x64}
if [ "$ARCH" = "x86" ]; then CXXBIN=$L/i686-w64-mingw32-clang++; OUT=Bin/imgod_editor_x86.exe
else CXXBIN=$L/x86_64-w64-mingw32-clang++; OUT=Bin/imgod_editor_x64.exe; fi
SRC="src/imgod.cpp src/json.cpp src/catalog.cpp src/model.cpp src/forms.cpp src/theme.cpp src/vsint.cpp src/codegen.cpp src/ui_editor.cpp src/canvas.cpp src/props.cpp src/editor_main.cpp"
IMG="$IMGUI_DIR/imgui.cpp $IMGUI_DIR/imgui_draw.cpp $IMGUI_DIR/imgui_tables.cpp $IMGUI_DIR/imgui_widgets.cpp $IMGUI_DIR/imgui_demo.cpp"
BACK="$IMGUI_DIR/imgui_impl_win32.cpp $IMGUI_DIR/imgui_impl_dx11.cpp $IMGUI_DIR/imgui_impl_dx9.cpp $IMGUI_DIR/imgui_impl_dx10.cpp $IMGUI_DIR/imgui_impl_opengl3.cpp"
mkdir -p Bin
$CXXBIN -O2 -std=c++17 -municode -mwindows -static -o $OUT $SRC $IMG $BACK \
   -Isrc -I$IMGUI_DIR -I$IMGUI_DIR/.. \
   -ld3d11 -ldxgi -ld3dcompiler -ld3d10 -ld3d9 -lopengl32 -ldwmapi -luuid -lole32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lshell32 -luser32 -ladvapi32
echo "OK: $OUT"
