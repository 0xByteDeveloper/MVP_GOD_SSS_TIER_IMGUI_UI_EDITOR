#!/bin/bash
# IMGUI_GOD_SSSTIER editör derlemesi (mingw-w64 GCC — x64)
# Kullanım: ./build.sh [x64]
set -e
cd "$(dirname "$0")"
DEB=${DEBROOT:-/home/z/my-project/tools/__debroot}
IMGUI_DIR=${IMGUI_DIR:-./imgui}
CXXBIN=$DEB/usr/bin/x86_64-w64-mingw32-g++-posix
OUT=Bin/imgod_editor_x64.exe
[ "$1" = "debug" ] && OUT=Bin/imgod_editor_dbg.exe
SRC="src/imgod.cpp src/json.cpp src/catalog.cpp src/model.cpp src/forms.cpp src/theme.cpp src/vsint.cpp src/codegen.cpp src/ui_editor.cpp src/canvas.cpp src/props.cpp src/objstyle.cpp src/editor_main.cpp"
IMG="$IMGUI_DIR/imgui.cpp $IMGUI_DIR/imgui_draw.cpp $IMGUI_DIR/imgui_tables.cpp $IMGUI_DIR/imgui_widgets.cpp $IMGUI_DIR/imgui_demo.cpp"
BACK="$IMGUI_DIR/imgui_impl_win32.cpp $IMGUI_DIR/imgui_impl_dx11.cpp $IMGUI_DIR/imgui_impl_dx9.cpp $IMGUI_DIR/imgui_impl_dx10.cpp $IMGUI_DIR/imgui_impl_opengl3.cpp"
mkdir -p Bin
rm -rf Bin/fonts
cp -r fonts Bin/fonts 2>/dev/null || true
DBG=0
[ "$1" = "debug" ] && DBG=1
GFLAG=""
[ "$DBG" = "1" ] && GFLAG="-g"
$CXXBIN -O2 $GFLAG -std=c++17 -municode -mwindows -static -o $OUT $SRC $IMG $BACK \
   -Isrc -I$IMGUI_DIR -DIMGUI_ASSERT_BT=1 \
   -ld3d11 -ldxgi -ld3dcompiler -ld3d10 -ld3d9 -lopengl32 -ldwmapi -luuid -lole32 -lgdi32 -lwinmm -limm32 -lcomdlg32 -lshell32 -luser32 -ladvapi32
echo "OK: $OUT"
