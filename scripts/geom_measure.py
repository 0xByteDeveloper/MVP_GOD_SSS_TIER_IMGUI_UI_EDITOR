#!/usr/bin/env python3
# geom_measure.py — ekran görüntüsünde: (a) 8 beyaz seçim tutamacı, (b) silik buton
# pikselleri, (c) form kâğıdı — tam sınırları ölçer.
import sys, struct
from collections import Counter

def load_png(path):
    # Basit PNG okuyucu (RGBA/RGB, renk tipleri 0,2,6) — yeterli
    import zlib
    with open(path,'rb') as f: data = f.read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    pos = 8; w=h=bd=0; idat=b''; ct=None
    while pos < len(data):
        ln = struct.unpack('>I', data[pos:pos+4])[0]
        typ = data[pos+4:pos+8]
        chunk = data[pos+8:pos+8+ln]
        if typ == b'IHDR':
            w,h,bd,ct = struct.unpack('>IIBB', chunk[:10])
        elif typ == b'IDAT': idat += chunk
        pos += 12 + ln
    raw = zlib.decompress(idat)
    ch = {0:1,2:3,6:4}[ct]
    stride = w*ch
    out = bytearray(w*h*4)
    prev = bytearray(stride)
    ptr = 0
    for y in range(h):
        flt = raw[ptr]; ptr += 1
        line = bytearray(raw[ptr:ptr+stride]); ptr += stride
        if flt==1:
            for i in range(ch, stride): line[i] = (line[i]+line[i-ch])&255
        elif flt==2:
            for i in range(stride): line[i]=(line[i]+prev[i])&255
        elif flt==3:
            for i in range(stride):
                a=line[i-ch] if i>=ch else 0; b=prev[i]
                line[i]=(line[i]+((a+b)>>1))&255
        elif flt==4:
            for i in range(stride):
                a=line[i-ch] if i>=ch else 0; b=prev[i]; c=prev[i-ch] if i>=ch else 0
                p=a+b-c; pa=abs(p-a); pb=abs(p-b); pc=abs(p-c)
                pr = a if (pa<=pb and pa<=pc) else (b if pb<=pc else c)
                line[i]=(line[i]+pr)&255
        prev = line
        for x in range(w):
            r,g,b = line[x*ch:x*ch+3]
            a = line[x*ch+3] if ch==4 else 255
            o=(y*w+x)*4; out[o]=r; out[o+1]=g; out[o+2]=b; out[o+3]=a
    return w,h,bytes(out)

def main(path):
    w,h,px = load_png(path)
    # 1) beyaz tutamaçlar: neredeyse saf beyaz (>=240,240,240)
    whites=[]; faint=[]; grid=[]
    for y in range(0,h,2):
        row=px[y*w*4:(y+1)*w*4]
        for x in range(0,w,2):
            r,g,b = row[x*4],row[x*4+1],row[x*4+2]
            if r>=240 and g>=240 and b>=240: whites.append((x,y))
            # silik buton: ImGui koyu themada buton gövdesi — mavi-gri yarı saydam
            # (60..110, 70..120, 95..145) — arka plan #23232D'den farklı, doygun değil
            if 55<=r<=115 and 65<=g<=125 and 90<=b<=150 and b>r: faint.append((x,y))
    def bbox(pts):
        if not pts: return None
        xs=[p[0] for p in pts]; ys=[p[1] for p in pts]
        return (min(xs),min(ys),max(xs),max(ys),len(pts))
    print(f"image {w}x{h}")
    print("white-handle pixels bbox:", bbox(whites))
    # tutamaç kümelerini 12px yakınlıkla kümele
    import math
    clusters=[]
    for p in whites:
        placed=False
        for c in clusters:
            if abs(p[0]-c[0][0])<=14 and abs(p[1]-c[0][1])<=14:
                c[1].append(p); placed=True; break
        if not placed: clusters.append(((p[0],p[1]),[p]))
    # yakın küme birleştirme (birkaç tur)
    for _ in range(4):
        merged=True
        while merged:
            merged=False
            for i in range(len(clusters)):
                for j in range(i+1,len(clusters)):
                    a=clusters[i][0]; b=clusters[j][0]
                    if abs(a[0]-b[0])<=20 and abs(a[1]-b[1])<=20:
                        clusters[i]=(a,clusters[i][1]+clusters[j][1]); clusters.pop(j); merged=True; break
                if merged: break
    print("handle clusters:")
    for c in clusters:
        xs=[q[0] for q in c[1]]; ys=[q[1] for q in c[1]]
        print(f"   center=({(min(xs)+max(xs))//2},{(min(ys)+max(ys))//2}) n={len(c[1])}")
    print("faint-button pixels bbox:", bbox(faint))
    if faint:
        xs=[p[0] for p in faint]; ys=[p[1] for p in faint]
        # en yoğun bölgeyi bul (histogram)
        print("   faint x range", min(xs), max(xs), " y range", min(ys), max(ys))

main(sys.argv[1])
