#!/bin/bash
# restore_toolchain.sh — kum kutusu geri dönünce (araçlar silinince) tüm
# derleme/çalıştırma zincirini geri kurar. Tek kullanım: bash restore_toolchain.sh
set -e
T=/home/z/my-project/tools
R=$T/__debroot
DEBS=/home/z/my-project/work/_debs
WINEDEB=/tmp/winedeb

# 1) mingw-w64 derleyicisi (.deb'ler work/_debs içinde kalıcı)
if [ ! -x $R/usr/bin/x86_64-w64-mingw32-g++-posix ]; then
  for d in $DEBS/*.deb; do dpkg -x "$d" $R/; done
fi

# 2) wine (libwine + wine64 + libz-mingw-w64) — ağ üzerinden indir
if [ ! -f $R/usr/lib/wine/wineserver64 ]; then
  mkdir -p $WINEDEB && cd $WINEDEB
  apt-get download libwine wine64 libz-mingw-w64 strace || true
  for d in $WINEDEB/*.deb; do dpkg -x "$d" $R/; done
fi

# 3) zlib1.dll — PE builtin dizinine + önek system32'ye
[ -f $R/usr/lib/x86_64-linux-gnu/wine/x86_64-windows/zlib1.dll ] || \
  cp $R/usr/x86_64-w64-mingw32/lib/zlib1.dll $R/usr/lib/x86_64-linux-gnu/wine/x86_64-windows/zlib1.dll
[ -f /home/z/my-project/work/.wine64/drive_c/windows/system32/zlib1.dll ] || \
  cp $R/usr/x86_64-w64-mingw32/lib/zlib1.dll /home/z/my-project/work/.wine64/drive_c/windows/system32/zlib1.dll

# 4) ntdll.so için exe-göreli dizin (symlink yasak → gerçek dosya kopyası gerekmez,
#    winex11/ntdll araması /proc/self/exe-göreli: wine64'ün yanına x86_64-unix gerek)
if [ ! -d $R/usr/lib/wine/x86_64-unix ]; then
  mkdir -p $R/usr/lib/wine/x86_64-unix
  for f in $R/usr/lib/x86_64-linux-gnu/wine/x86_64-unix/*.so; do
    ln -sf "$f" $R/usr/lib/wine/x86_64-unix/ 2>/dev/null || cp "$f" $R/usr/lib/wine/x86_64-unix/
  done
fi

# 5) wineserver sarmalayıcı (Debian betiği kök-yollarına bakıyor → gerçek ELF'e yönlendir)
printf '#!/bin/sh\nexec %s/usr/lib/wine/wineserver64 "$@"\n' "$R" > $R/usr/lib/wine/wineserver
chmod +x $R/usr/lib/wine/wineserver

# 6) LD_PRELOAD nls-yönlendirme kancası (kaynak kalıcı: scripts/wine_nls_redirect.c)
gcc -shared -fPIC -O2 -o $T/bin/wine_nls_redirect.so /home/z/my-project/scripts/wine_nls_redirect.c -ldl

# 7) wine64 sarmalayıcı betiği
mkdir -p $T/bin
cat > $T/bin/wine64 <<'EOF'
#!/bin/bash
T=/home/z/my-project/tools
R=$T/__debroot
export LD_LIBRARY_PATH=$R/usr/lib/x86_64-linux-gnu:$R/usr/lib/x86_64-linux-gnu/wine${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export WINEDLLDIR=$R/usr/lib/x86_64-linux-gnu/wine
export WINEDLLPATH=$R/usr/lib/x86_64-linux-gnu/wine
export WINEDATADIR=$R/usr/share/wine
export WINESERVER=$R/usr/lib/wine/wineserver64
export WINEPREFIX=${WINEPREFIX:-/home/z/my-project/work/.wine64}
export LD_PRELOAD=$T/bin/wine_nls_redirect.so${LD_PRELOAD:+:$LD_PRELOAD}
exec $R/usr/lib/wine/wine64 "$@"
EOF
chmod +x $T/bin/wine64
echo "OK: araç zinciri hazır — $R/usr/bin/x86_64-w64-mingw32-g++-posix + $T/bin/wine64"
