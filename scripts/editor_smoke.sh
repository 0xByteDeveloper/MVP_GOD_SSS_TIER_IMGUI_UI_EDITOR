#!/bin/bash
# editor_smoke.sh — editörü Wine+Xvfb altında çalıştır, ekran görüntüsü al
# kullanım: editor_smoke.sh <outpng> <sleep_sec> [komut satırı argümanları...]
set -u
OUT="$1"; SLEEP="${2:-10}"; shift 2 2>/dev/null || true
export WINEPREFIX=/home/z/my-project/work/.wine64
export WINE=/home/z/my-project/tools/bin/wine64
export WINESERVER=/home/z/my-project/tools/bin/wineserver64
export DISPLAY=:97
rm -f "$OUT"
pkill -f "Xvfb :97" 2>/dev/null; sleep 0.3
Xvfb :97 -screen 0 1680x960x24 > /tmp/xvfb_smoke.log 2>&1 &
XPID=$!
sleep 1.5
cd /home/z/my-project/work/god_editor/Bin
"$WINE" ./imgod_editor_x64.exe "$@" > /tmp/wine_editor.log 2>&1 &
WPID=$!
sleep "$SLEEP"
ffmpeg -y -loglevel error -f x11grab -video_size 1680x960 -i :97 -frames:v 1 "$OUT" 2>>/tmp/wine_editor.log
kill $WPID 2>/dev/null
sleep 0.8
/home/z/my-project/tools/bin/wineserver64 -k 2>/dev/null
kill $XPID 2>/dev/null
echo "shot: $OUT ($(stat -c%s "$OUT" 2>/dev/null) bytes)"
