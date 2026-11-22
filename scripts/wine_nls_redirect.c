// wine_nls_redirect.c — LD_PRELOAD kancası: Debian wine ikililerinin
// sert kodlu /usr/share/wine/... ve /usr/local/share/wine/... yollarını
// kullanıcı ağacına (/home/z/my-project/tools/__debroot/usr/share/wine/...)
// yönlendirir. Kök yetkisi gerektirmeyen kullanıcı düzeyi çözüm.
// v2: "/usr/share/wine/wine/nls" (wineserver çift-kök yolu) ve
//     "/usr/lib/wine/", "/usr/lib/x86_64-linux-gnu/wine" (derleme içi
//     öntanımlı yollar) da yönlendirilir.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* newroot = "/home/z/my-project/tools/__debroot";
static __thread char buf[PATH_MAX];

// /usr/share/wine/wine/nls  →  newroot/usr/share/wine/nls   (wineserver çift-kök)
// /usr/share/wine/X          →  newroot/usr/share/wine/X
// /usr/local/share/wine/X    →  newroot/usr/share/wine/X     (wine yolu taşınır)
// /usr/lib/wine/X            →  newroot/usr/lib/wine/X
// /usr/lib/x86_64-linux-gnu/wine/X → newroot/usr/lib/x86_64-linux-gnu/wine/X
static const char* redirect(const char* path) {
    if (!path) return path;
    if (strncmp(path, "/usr/share/wine/wine/", 22) == 0) {
        snprintf(buf, sizeof(buf), "%s/usr/share/wine/%s", newroot, path + 22);
        return buf;
    }
    if (strncmp(path, "/usr/share/wine/", 16) == 0) {
        snprintf(buf, sizeof(buf), "%s%s", newroot, path);
        return buf;
    }
    if (strncmp(path, "/usr/local/share/wine/", 22) == 0) {
        snprintf(buf, sizeof(buf), "%s/usr/share/wine/%s", newroot, path + 22);
        return buf;
    }
    if (strncmp(path, "/usr/lib/wine/", 15) == 0) {
        snprintf(buf, sizeof(buf), "%s%s", newroot, path);
        return buf;
    }
    if (strncmp(path, "/usr/lib/x86_64-linux-gnu/wine/", 31) == 0) {
        snprintf(buf, sizeof(buf), "%s%s", newroot, path);
        return buf;
    }
    return path;
}

int open(const char* path, int flags, ...) {
    static int (*real)(const char*, int, ...) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "open");
    mode_t mode = 0;
    va_list ap; va_start(ap, flags);
    if (flags & (O_CREAT | O_TMPFILE)) mode = (mode_t)va_arg(ap, int);
    va_end(ap);
    return real(redirect(path), flags, mode);
}

int openat(int dirfd, const char* path, int flags, ...) {
    static int (*real)(int, const char*, int, ...) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "openat");
    mode_t mode = 0;
    va_list ap; va_start(ap, flags);
    if (flags & (O_CREAT | O_TMPFILE)) mode = (mode_t)va_arg(ap, int);
    va_end(ap);
    return real(dirfd, redirect(path), flags, mode);
}

FILE* fopen(const char* path, const char* mode) {
    static FILE* (*real)(const char*, const char*) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "fopen");
    return real(redirect(path), mode);
}

int stat(const char* path, struct stat* st) {
    static int (*real)(const char*, struct stat*) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "stat");
    return real(redirect(path), st);
}
int __xstat(int ver, const char* path, struct stat* st) {
    static int (*real)(int, const char*, struct stat*) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "__xstat");
    return real(ver, redirect(path), st);
}
int access(const char* path, int mode) {
    static int (*real)(const char*, int) = NULL;
    if (!real) real = dlsym(RTLD_NEXT, "access");
    return real(redirect(path), mode);
}
