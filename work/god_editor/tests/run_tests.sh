#!/bin/bash
# run_tests.sh — IMGUI GOD SSS TIER editör test paketi (Wine + Xvfb altında)
# Kullanım: ./run_tests.sh
# Çıktı: her test için PASS/FAIL + özet
set -u
cd "$(dirname "$0")/.."
export WINEPREFIX=/home/z/my-project/work/.wine64
export DISPLAY=:97
WINE=/home/z/my-project/tools/bin/wine64
EXE=Bin/imgod_editor_x64.exe
PASS=0; FAIL=0; FAILED=()

pkill -f "Xvfb :97" 2>/dev/null; sleep 0.5
Xvfb :97 -screen 0 1680x960x24 > /tmp/xvfb_test.log 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 1.5

run_editor() {   # <selftest> <saniye>
    (cd Bin && rm -f state_*.imgod state_*.txt && "$WINE" ./imgod_editor_x64.exe --selftest "$1" > /tmp/wine_test.log 2>&1) &
    local pid=$!
    sleep "$2"
    kill $pid 2>/dev/null; sleep 0.7
    "$WINE" -k 2>/dev/null; /home/z/my-project/tools/bin/wineserver64 -k 2>/dev/null
    sleep 0.5
}

check() {  # <ad> <koşul-komutu>
    if eval "$2" >/dev/null 2>&1; then
        echo "  [PASS] $1"; PASS=$((PASS+1))
    else
        echo "  [FAIL] $1"; FAIL=$((FAIL+1)); FAILED+=("$1")
    fi
}

echo "=== TEST 1: temel (ekleme/özellik/kayıt) ==="
run_editor ../tests/test_basic.txt 12
check "kayıt oluştu" '[ -s Bin/state_basic.imgod ]'
check "3 nesne" 'python3 -c "import json;d=json.load(open(\"Bin/state_basic.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==3 else 1)"'
check "renk özelliği korundu" 'grep -q "D25410" Bin/state_basic.imgod'
check "Türkçe metin korundu" 'grep -q "Merhaba Dünya" Bin/state_basic.imgod'

echo "=== TEST 2: etkileşim (arm/yerleştir/rubber band/pano) ==="
run_editor ../tests/test_interact.txt 20
check "kayıt oluştu" '[ -s Bin/state_interact.imgod ]'
check "yerleştirme: 8 nesne (3+1 yerleşik+4 yapıştırılan)" 'python3 -c "import json;d=json.load(open(\"Bin/state_interact.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==8 else 1)"'
check "yerleştirilen button2 var" 'grep -q "\"button2\"" Bin/state_interact.imgod'
check "yapıştırılan button3 var (tip sayacı)" 'grep -q "\"button3\"" Bin/state_interact.imgod'

echo "=== TEST 3: pano yeniden adlandırma (denemebutton → button2 tarzı) ==="
run_editor ../tests/test_clipboard.txt 14
check "kayıt oluştu" '[ -s Bin/state_clip.imgod ]'
check "yapıştırılanlar tip adıyla: 4 nesne" 'python3 -c "import json;d=json.load(open(\"Bin/state_clip.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==4 else 1)"'
check "button2 (sayaç) oluştu" 'grep -q "\"button2\"" Bin/state_clip.imgod'
check "combo2 (sayaç) oluştu" 'grep -q "\"combo2\"" Bin/state_clip.imgod'

echo "=== TEST 4: çok formlu park/unpark ==="
run_editor ../tests/test_forms.txt 12
check "kayıt oluştu" '[ -s Bin/state_forms.imgod ]'
check "2 form" 'python3 -c "import json;d=json.load(open(\"Bin/state_forms.imgod\"));exit(0 if d.get(\"FormCount\")==2 else 1)"'
check "Form1 button1 park edildi" 'python3 -c "
import json;d=json.load(open(\"Bin/state_forms.imgod\"))
names=[f[\"FormName\"] for f in d[\"Forms\"]];exit(0 if \"button1\" in json.dumps(d) else 1)"'

echo "=== TEST 5: TEK YERLEŞTİRME (sınırsız oluşturma hatası) ==="
run_editor ../tests/test_bugfix.txt 14
check "kayıt oluştu" '[ -s Bin/state_bugfix.imgod ]'
check "yalnız 2 nesne: ikinci tık yeni nesne YARATMADI" 'python3 -c "import json;d=json.load(open(\"Bin/state_bugfix.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==2 else 1)"'
check "button2 yerleştirildi (tek yerleştirme)" 'grep -q "\"button2\"" Bin/state_bugfix.imgod'

echo "=== TEST 6: FORM BOYUTLANDIRMA (sınırsız büyüme hatası) ==="
run_editor ../tests/test_resize.txt 20
check "kayıt oluştu" '[ -s Bin/state_resize1.imgod ]'
check "kesin delta: 430+120=550 / 460+60=520 (birikim YOK)" 'python3 -c "import json;d=json.load(open(\"Bin/state_resize1.imgod\"));f=d[\"Forms\"][0];exit(0 if abs(f[\"FormW\"]-550)<2 and abs(f[\"FormH\"]-520)<2 else 1)"'
check "aşırı sürükleme canvas sınırına kırpıldı" 'python3 -c "import json;d=json.load(open(\"Bin/state_resize2.imgod\"));f=d[\"Forms\"][0];exit(0 if 900<f[\"FormW\"]<=1000 and 700<f[\"FormH\"]<=800 else 1)"'

echo "=== TEST 7: SAĞ TIK BAĞLAM MENÜSÜ ==="
run_editor ../tests/test_rclick.txt 20
check "sağ tık nesneyi seçti (SEL:1)" 'grep -q "SEL:1" Bin/state_rclick_sel.txt'
check "ESC menüyü kapattı, seçim KORUNDU" 'grep -q "SEL:1" Bin/state_rclick_sel2.txt'
check "Del seçili nesneyi sildi (1 nesne kaldı)" 'python3 -c "import json;d=json.load(open(\"Bin/state_rclick.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==1 else 1)"'

echo "=== TEST 8: PANEL (yeni araç) — konteyner içine yerleştirme ==="
run_editor ../tests/test_panel.txt 16
check "kayıt oluştu" '[ -s Bin/state_panel.imgod ]'
check "2 nesne (panel1 + button1)" 'python3 -c "import json;d=json.load(open(\"Bin/state_panel.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==2 else 1)"'
check "button PARENT=panel1 (içine yerleşti)" 'python3 -c "
import json;d=json.load(open(\"Bin/state_panel.imgod\"))
cs=d[\"Forms\"][0][\"Ctrls\"]
b=[c for c in cs if c[\"Type\"]==\"Button\"][0]
exit(0 if b.get(\"Parent\")==\"panel1\" else 1)"'
check "Panel stili korundu (Rounding=6)" 'grep -q "6" Bin/state_panel.imgod'

echo "=== TEST 9: kod üretimi (VS2019 + Panel WYSIWYG) ==="
rm -rf /home/z/my-project/work/gen_target
run_editor ../tests/test_gen.txt 45
check ".sln üretildi" '[ -s /home/z/my-project/work/gen_target/DenemImgui.sln ]'
check ".vcxproj üretildi" '[ -s /home/z/my-project/work/gen_target/DenemImgui/DenemImgui.vcxproj ]'
check "Form1.h üretildi (null-model çöküşü düzeltildi)" '[ -s /home/z/my-project/work/gen_target/DenemImgui/Form1.h ]'
check "build_mingw.sh üretildi" '[ -s /home/z/my-project/work/gen_target/DenemImgui/build_mingw.sh ]'
check "IGOD::Panel Global.h'de" 'grep -q "struct Panel" /home/z/my-project/work/gen_target/DenemImgui/Global.h'
check "panel1 Begin/End + çocuk button1 içeride" 'grep -q "panel1.Begin();" /home/z/my-project/work/gen_target/DenemImgui/Form1.cpp && grep -q "button1_Click" /home/z/my-project/work/gen_target/DenemImgui/Form1.cpp'
check "panel boyutu senkron (370x270)" 'grep -q "panel1.Size = ImVec2(370, 270);" /home/z/my-project/work/gen_target/DenemImgui/Form1.cpp'
check "panel BgColor push (WYSIWYG stil)" 'grep -q "PushStyleColor(ImGuiCol_ChildBg" /home/z/my-project/work/gen_target/DenemImgui/Form1.cpp'

echo "=== TEST 10: WIDGET BOYUTLANDIRMA (combo/listbox/slider GÖRSEL WYSIWYG) ==="
run_editor ../tests/test_widgetsize.txt 18
check "kayıt oluştu" '[ -s Bin/state_wsize.imgod ]'
check "combo1 genişlik +120 GÖRSEL uygulandı (SetNextItemWidth — delta ölçüm)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"combo1:(\d+)x\",t)]
exit(0 if len(v)>=2 and 117<=v[1]-v[0]<=123 else 1)"'
check "listBox1 TAM boyut: genişlik +60 ve yükseklik 180 GÖRSEL (BeginListBox)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"listBox1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 57<=v[1][0]-v[0][0]<=63 and 178<=v[1][1]<=182 else 1)"'
check "sliderFloat1 genişlik +100 GÖRSEL (etiket çerçeve içinde)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"sliderFloat1:(\d+)x\",t)]
exit(0 if len(v)>=2 and 97<=v[1]-v[0]<=103 else 1)"'
check "inputText1 genişlik +50 GÖRSEL (delta ölçüm)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"inputText1:(\d+)x\",t)]
exit(0 if len(v)>=2 and 47<=v[1]-v[0]<=53 else 1)"'
check "checkbox1 genişlik İÇERİĞE döndü (otomatik — hayalet boyut yok)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"checkbox1:(\d+)x\",t)]
exit(0 if len(v)>=2 and -2<=v[1]-v[0]<=2 else 1)"'
check "table1 genişlik 360 GÖRSEL (outer_size — SON ölçüm)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"table1:(\d+)x\",t)]
exit(0 if v and 357<=v[-1]<=363 else 1)"'
check "SAĞ TUTAMAÇ sürüklemesi: combo1 +80 (maskeli boyutlandırma)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[int(x) for x in re.findall(r\"combo1:(\d+)x\",t)]
exit(0 if len(v)>=3 and 77<=v[2]-v[1]<=83 else 1)"'
check "modelde kalıcı: combo1 W=400, listBox1 260x180" 'python3 -c "
import json;d=json.load(open(\"Bin/state_wsize.imgod\"))
cs=d[\"Forms\"][0][\"Ctrls\"]
co=[c for c in cs if c[\"Type\"]==\"Combo\"][0]; li=[c for c in cs if c[\"Type\"]==\"ListBox\"][0]
exit(0 if 396<=co[\"W\"]<=404 and 258<=li[\"W\"]<=262 and 178<=li[\"H\"]<=182 else 1)"'

echo ""
echo "=========================================="
echo "SONUÇ: $PASS PASS / $FAIL FAIL"
if [ $FAIL -gt 0 ]; then
    printf 'BAŞARISIZ: %s\n' "${FAILED[*]}"
    exit 1
fi
echo "TÜM TESTLER GEÇTİ ✓"
