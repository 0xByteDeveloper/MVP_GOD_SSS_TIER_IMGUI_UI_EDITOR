// x11snap.c — Xvfb kök penceresini XSHM'siz GetImage ile BMP'ye yazar
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "kullanim: %s <cikti.bmp> [ekran]\n", argv[0]); return 1; }
    const char* disp = argc > 2 ? argv[2] : ":97";
    Display* d = XOpenDisplay(disp);
    if (!d) { fprintf(stderr, "X acilamadi: %s\n", disp); return 2; }
    int root = DefaultRootWindow(d);
    unsigned int w, h;
    { Window rw; int x, y; unsigned int bw, depth;
      XGetGeometry(d, root, &rw, &x, &y, &w, &h, &bw, &depth); }
    XImage* img = XGetImage(d, root, 0, 0, w, h, AllPlanes, ZPixmap);
    if (!img) { fprintf(stderr, "XGetImage basarisiz\n"); XCloseDisplay(d); return 3; }
    // BMP (24-bit, üstten aşağı)
    int stride = ((int)w * 3 + 3) & ~3;
    unsigned char* px = (unsigned char*)malloc((size_t)stride * h);
    unsigned long rMask = img->red_mask, gMask = img->green_mask, bMask = img->blue_mask;
    int rShift = 0, gShift = 0, bShift = 0;
    { // maskeler LSB hizalı değilse kaydır hesapla
      unsigned long m;
      m = rMask; while (m && !(m & 1)) { m >>= 1; rShift++; }
      m = gMask; while (m && !(m & 1)) { m >>= 1; gShift++; }
      m = bMask; while (m && !(m & 1)) { m >>= 1; bShift++; }
    }
    int bpp = img->bits_per_pixel;
    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) {
            unsigned long p = 0;
            if (bpp == 32) p = *(unsigned long*)((char*)img->data + y * img->bytes_per_line + (size_t)x * 4);
            else if (bpp == 16) p = *(unsigned short*)((char*)img->data + y * img->bytes_per_line + (size_t)x * 2);
            else p = *(unsigned char*)((char*)img->data + y * img->bytes_per_line + x);
            int r = (int)((p & rMask) >> rShift);
            int g = (int)((p & gMask) >> gShift);
            int b = (int)((p & bMask) >> bShift);
            if (img->depth <= 16) { r = r * 255 / 31; g = g * 255 / 63; b = b * 255 / 31; }
            unsigned char* dst = px + (size_t)y * stride + x * 3;
            dst[0] = (unsigned char)b; dst[1] = (unsigned char)g; dst[2] = (unsigned char)r;
        }
    }
    typedef struct { unsigned short type; unsigned int size, r1, off; } __attribute__((packed)) FH;
    typedef struct { unsigned int hs, w, h; unsigned short pl, bpp; unsigned int comp, imgsz, xppm, yppm, clr, clrImp; } __attribute__((packed)) IH;
    FH fh = { 0x4D42, 54 + (unsigned)(stride * h), 0, 54 };
    IH ih = { 40, (unsigned)w, (unsigned)h, 1, 24, 0, (unsigned)(stride * h), 2835, 2835, 0, 0 };
    FILE* f = fopen(argv[1], "wb");
    if (!f) { fprintf(stderr, "dosya acilamadi\n"); return 4; }
    fwrite(&fh, 1, sizeof fh, f); fwrite(&ih, 1, sizeof ih, f); fwrite(px, 1, (size_t)stride * h, f);
    fclose(f);
    XDestroyImage(img);
    XCloseDisplay(d);
    printf("OK: %s (%ux%u)\n", argv[1], w, h);
    return 0;
}
