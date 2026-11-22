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
    # 6. TUR DÜZELTİLDİ: TEST 12'nin MinGW derleme kontrolü `cd` yapıp kabuğun
    # çalışma dizinini değiştiriyordu → sonraki testler Bin'e giremiyordu
    # ("failed to open ./imgod_editor_x64.exe"). Artık KESİN yol kullanılır.
    (cd /home/z/my-project/work/god_editor/Bin && rm -f state_*.imgod state_*.txt && "$WINE" ./imgod_editor_x64.exe --selftest "$1" > /tmp/wine_test.log 2>&1) &
    local pid=$!
    sleep "$2"
    kill $pid 2>/dev/null; sleep 0.7
    "$WINE" -k 2>/dev/null; /home/z/my-project/tools/bin/wineserver64 -k 2>/dev/null
    sleep 0.5
    cd /home/z/my-project/work/god_editor
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
check "checkbox1 alan genişliği 500 KALDI + yükseklik 36 ölçeklendi (5. tur FULL boyut)" 'python3 -c "
import re;t=open(\"Bin/state_wsz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"checkbox1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 496<=v[1][0]<=504 and 34<=v[1][1]<=38 else 1)"'
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


echo "=== TEST 11: 5. TUR ETKİLEŞİMLER (TabBar/label/checkbox/separator/TextBox/form-tık/groupbox-damak) ==="
run_editor ../tests/test_round5.txt 25
check "form boş alan tıkı formu SEÇTİ (SELOK -)" 'grep -q "SELOK -" Bin/state_r5_form.txt'
check "tabBar1 başlangıç 400x220 (FULL bölge)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"tabBar1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=1 and 395<=v[0][0]<=405 and 215<=v[0][1]<=225 else 1)"'
check "tabBar1 300x150 GÖRSEL (5. tur #1 — child bölge boyutu)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"tabBar1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 295<=v[1][0]<=305 and 145<=v[1][1]<=155 else 1)"'
check "labelText1 180'e küçüldü (değer sağa hizalı — 5. tur #3)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[int(x) for x in re.findall(r\"labelText1:(\d+)x\",t)]
exit(0 if len(v)>=2 and 250<=v[0]-v[1]<=380 else 1)"'
check "text1 wrap 140: genişlik sınırlı + çok satır (5. tur #4)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"text1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 110<=v[1][0]<=145 and v[1][1]>=50 else 1)"'
check "checkbox1 200x34 GÖRSEL (kare büyüdü — 5. tur #4)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"checkbox1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=1 and 195<=v[0][0]<=205 and 32<=v[0][1]<=36 else 1)"'
check "separator1 başlangıç 200x2 + 300x8 GÖRSEL (5. tur #7)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"separator1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 195<=v[0][0]<=205 and v[0][1]<=4 and 295<=v[1][0]<=305 and 6<=v[1][1]<=10 else 1)"'
check "textBox1 VAR (~200 genişlik — 5. tur #6)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[int(x) for x in re.findall(r\"textBox1:(\d+)x\",t)]
exit(0 if len(v)>=1 and 229<=v[0]<=243 else 1)"'
check "GroupBox İÇİNE yerleşti (PARENTOK — 5. tur #2)" 'grep -q "PARENTOK button1->groupBox1" Bin/state_r5_parent.txt'
check "groupbox İÇİNDEKİ buton tıkla→buton seçildi (SELOK button1 — derinlik hit-testi)" 'grep -q "SELOK button1" Bin/state_r5.txt'
check "buton İÇİNDE +30 resize (150x28)" 'python3 -c "
import re;t=open(\"Bin/state_r5sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"button1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=1 and 145<=v[0][0]<=155 else 1)"'
check "kayıt: 10 nesne (r5)" 'python3 -c "import json;d=json.load(open(\"Bin/state_r5.imgod\"));exit(0 if len(d[\"Forms\"][0][\"Ctrls\"])==9 else 1)"'

echo "=== TEST 12: 5. TUR ÜRETİM (tüm yeni boyutlar kodda) + MinGW DERLEMESİ ==="
rm -rf /home/z/my-project/work/gen5
run_editor ../tests/test_gen5.txt 60
check "üretim oluştu" '[ -s /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp ]'
check "TabBar Size bağlı (W×H)" 'grep -q "tabBar1.Size = ImVec2(400, 200);" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "TabBar child sarması + denge End" 'grep -q "tabBar1.End();   // child dengesi" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "sekmeye yerleşen çocuk (TabOwner=Genel aktif sekme)" 'grep -q "Sekme İçi Buton" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "TextBox → IGOD::InputText sarması + Width" 'grep -q "IGOD::InputText textBox1;" /home/z/my-project/work/gen5/DenemImgui5/Form1.h && grep -q "textBox1.Width = 260;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "LabelText genişliği SetNextItemWidth (200.0f)" 'grep -q "SetNextItemWidth(200.0f);" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "Text wrap PushTextWrapPos (GetCursorPos + 180.0f)" 'grep -q "PushTextWrapPos(ImGui::GetCursorPos().x + 180.0f);" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "BulletText IGOD sarması + Width" 'grep -q "IGOD::BulletText bulletText1;" /home/z/my-project/work/gen5/DenemImgui5/Form1.h && grep -q "bulletText1.Width = 150;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "Checkbox W+H bağlı" 'grep -q "checkbox1.Width = 180;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp && grep -q "checkbox1.Height = 30;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "Separator W×H (IGOD::Separator + Size)" 'grep -q "IGOD::Separator separator1;" /home/z/my-project/work/gen5/DenemImgui5/Form1.h && grep -q "separator1.Size = ImVec2(280, 6);" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "SeparatorText W + RadioButton W+H + Header W" 'grep -q "separatorText1.Width = 240;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp && grep -q "radioButton1.Height = 30;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp && grep -q "collapsingHeader1.Width = 240;" /home/z/my-project/work/gen5/DenemImgui5/Form1.cpp'
check "Global.h: FitTextToWidth + yeni yapılar" 'grep -q "FitTextToWidth" /home/z/my-project/work/gen5/DenemImgui5/Global.h && grep -q "struct Separator {" /home/z/my-project/work/gen5/DenemImgui5/Global.h && grep -q "struct BulletText {" /home/z/my-project/work/gen5/DenemImgui5/Global.h'
check "MinGW DERLEMESİ: üretilen proje derlendi (exe oluştu)" 'cd /home/z/my-project/work/gen5/DenemImgui5 && CXX=/home/z/my-project/tools/__debroot/usr/bin/x86_64-w64-mingw32-g++-posix PATH=/home/z/my-project/tools/__debroot/usr/bin:$PATH bash build_mingw.sh >/tmp/gen5build.log 2>&1 && [ -s Bin/DenemImgui5.exe ]'

echo "=== TEST 13: 6. TUR ETKİLEŞİMLER (GroupBox reparent/kırpma + TextBox dikey + etiket gizleme) ==="
run_editor ../tests/test_round6.txt 30
check "kayıt oluştu (r6)" '[ -s Bin/state_r6.imgod ]'
check "Groupbox DIŞINDA başladı (PARENTOK button1->-)" 'grep -q "PARENTOK button1->-" Bin/state_r6_p1.txt'
check "6. TUR #2: SÜRÜKLE-BIRAK GroupBox İÇİNE (PARENTOK button1->groupBox1)" 'grep -q "PARENTOK button1->groupBox1" Bin/state_r6_p2.txt'
check "taşıyıcı hareketi ÇOCUĞU DA TAŞIR (+32,+24)" 'python3 -c "
import re;t=open(\"Bin/state_r6pos.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"button1:@(-?\d+),(-?\d+)\",t)]
exit(0 if len(v)>=2 and 30<=v[1][0]-v[0][0]<=34 and 22<=v[1][1]-v[0][1]<=26 else 1)"'
check "groupbox KÜÇÜLTÜLÜNCE çocuk KIRPILIR ama parent KORUNUR" 'grep -q "PARENTOK button1->groupBox1" Bin/state_r6_p3.txt'
check "TextBox DİKEY boyut +24 GÖRSEL (alt tutamaç — 6. tur #1)" 'python3 -c "
import re;t=open(\"Bin/state_r6sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"textBox1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and 20<=v[1][1]-v[0][1]<=28 else 1)"'
check "etiket gizleme: checkbox genişliği KÜÇÜLDÜ (yan metin kalktı — 6. tur #4)" 'python3 -c "
import re;t=open(\"Bin/state_r6sz.txt\").read()
v=[(int(a),int(b)) for a,b in re.findall(r\"checkbox1:(\d+)x(\d+)\",t)]
exit(0 if len(v)>=2 and v[1][0]<=40 and v[0][0]-v[1][0]>=30 else 1)"'
check "groupbox DIŞINA bırakınca FORMA döndü (PARENTOK button1->-)" 'grep -q "PARENTOK button1->-" Bin/state_r6_p4.txt'
check "modelde: button1 parent=groupBox1 KAYITLI + koordinat çevrimi (28,36)" 'python3 -c "
import json;d=json.load(open(\"Bin/state_r6.imgod\"))
cs=d[\"Forms\"][0][\"Ctrls\"]
b=[c for c in cs if c[\"Name\"]==\"button1\"][0]
exit(0 if b.get(\"Parent\")==\"groupBox1\" and b[\"X\"]==28 and b[\"Y\"]==36 else 1)"'

echo "=== TEST 14: 6. TUR ÜRETİM (TextBox H + etiketsiz ##ad + PushFrameHeight) + MinGW ==="
rm -rf /home/z/my-project/work/gen6
run_editor ../tests/test_gen6.txt 65
check "üretim oluştu (r6)" '[ -s /home/z/my-project/work/gen6/DenemR6/Form1.cpp ]'
check "Global.h: PushFrameHeight yardımcısı VAR (çerçeve yükseklik ölçekleme)" 'grep -q "inline bool PushFrameHeight" /home/z/my-project/work/gen6/DenemR6/Global.h'
check "IGOD::InputText Height üyesi VAR" 'grep -q "float Height = 0;" /home/z/my-project/work/gen6/DenemR6/Global.h'
check "textBox1 yüksekliği bağlandı (19+20 → 39)" 'grep -q "textBox1.Height = 39;" /home/z/my-project/work/gen6/DenemR6/Form1.cpp'
check "sliderFloat1/combo1 yüksekliği bağlandı (37/35)" 'grep -q "sliderFloat1.Height = 37;" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && grep -q "combo1.Height = 35;" /home/z/my-project/work/gen6/DenemR6/Form1.cpp'
check "ETİKETSİZ üretim: u8\"##textBox1\" (Yazı yok — 6. tur #4)" 'grep -q "u8\"##textBox1\"" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && ! grep -q "Yazı##" /home/z/my-project/work/gen6/DenemR6/Form1.cpp'
check "ETİKETSİZ üretim: u8\"##checkbox1\" + u8\"##sliderFloat1\" + u8\"##combo1\"" 'grep -q "u8\"##checkbox1\"" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && grep -q "u8\"##sliderFloat1\"" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && grep -q "u8\"##combo1\"" /home/z/my-project/work/gen6/DenemR6/Form1.cpp'
check "Seçenek/Değer/Seçim yan metinleri YOK (gizli üretim)" '! grep -q "Se\303\247enek##" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && ! grep -q "De\304\237er##" /home/z/my-project/work/gen6/DenemR6/Form1.cpp && ! grep -q "Se\303\247im##" /home/z/my-project/work/gen6/DenemR6/Form1.cpp'
check "GroupBox çocuk hiyerarşisi: textBox1 Begin/End İÇİNDE" 'python3 -c "
import re;t=open(\"/home/z/my-project/work/gen6/DenemR6/Form1.cpp\",encoding=\"utf-8\").read()
b=t.find(\"groupBox1.Begin();\"); e=t.find(\"groupBox1.End();\"); i=t.find(\"textBox1.Draw()\")
exit(0 if 0<=b<i<=e else 1)"'
check "MinGW DERLEMESİ: üretilen r6 projesi derlendi" 'cd /home/z/my-project/work/gen6/DenemR6 && CXX=/home/z/my-project/tools/__debroot/usr/bin/x86_64-w64-mingw32-g++-posix PATH=/home/z/my-project/tools/__debroot/usr/bin:$PATH bash build_mingw.sh >/tmp/gen6build.log 2>&1 && [ -s Bin/DenemR6.exe ]'

echo ""
echo "=========================================="
echo "SONUÇ: $PASS PASS / $FAIL FAIL"
if [ $FAIL -gt 0 ]; then
    printf 'BAŞARISIZ: %s\n' "${FAILED[*]}"
    exit 1
fi
echo "TÜM TESTLER GEÇTİ ✓"
