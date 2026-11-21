#!/bin/bash
# wine_smoke.sh — editörü Wine+Xvfb altında çalıştır, ekran görüntüsü al
# kullanım: wine_smoke.sh <exe> <outpng> <sleep_sec> [komut satırı argümanları...]
set -u
EXE="$1"; OUT="$2"; SLEEP="${3:-12}"; shift 3 2>/dev/null || shift $#
export WINEPREFIX=/home/z/my-project/work/.wine64
export WINE=/home/z/my-project/tools/bin/wine64
export WINESERVER=/home/z/my-project/tools/__debroot/usr/lib/wine/wineserver64
export WINEDLLDIR=/home/z/my-project/tools/__debroot/usr/lib/x86_64-linux-gnu/wine
export WINEDATADIR=/home/z/my-project/tools/__debroot/usr/share/wine
export DISPLAY=:97
rm -f "$OUT"
Xvfb :97 -screen 0 1280x840x24 > /tmp/xvfb_smoke.log 2>&1 &
XPID=$!
sleep 2
cd "$(dirname "$OUT")"
"$WINE" "$EXE" "$@" > /tmp/wine_smoke.log 2>&1 &
WPID=$!
sleep "$SLEEP"
ffmpeg -y -loglevel error -f x11grab -video_size 1280x840 -i :97 -frames:v 1 "$OUT" 2>>/tmp/wine_smoke.log
kill $WPID 2>/dev/null
sleep 1
/home/z/my-project/tools/bin/wineserver64 -k 2>/dev/null
kill $XPID 2>/dev/null
echo "shot: $OUT ($(stat -c%s "$OUT" 2>/dev/null) bytes)"
