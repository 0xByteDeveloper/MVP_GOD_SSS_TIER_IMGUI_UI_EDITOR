#!/usr/bin/env python3
"""Recursively download .deb packages + deps and extract them into a local prefix (no root)."""
import subprocess, sys, os, re

PREFIX = "/home/z/my-project/tools"
CACHEDIR = "/tmp/debs"

def depends(pkg):
    out = subprocess.run(["apt-cache", "depends", pkg], capture_output=True, text=True).stdout
    deps = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Depends:"):
            d = line[len("Depends:"):].strip()
            if not d or d.startswith("<") or "|" in d:
                continue
            m = re.match(r"^([a-z0-9.+\-]+)", d)
            if m:
                deps.append(m.group(1))
    return deps

SKIP = {
    "libc6", "libc-bin", "dpkg", "install-info", "lsb-base", "debconf",
    "perl", "perl-base", "gcc-14-base", "libgcc-s1", "libgcc-14",
    "libstdc++6", "adduser", "passwd", "systemd", "libselinux1",
}

def main():
    os.makedirs(CACHEDIR, exist_ok=True)
    roots = sys.argv[1:]
    seen = set()
    pkgs = []
    todo = list(roots)
    while todo:
        p = todo.pop(0)
        if p in seen or p in SKIP:
            continue
        seen.add(p)
        pkgs.append(p)
        for d in depends(p):
            if d not in seen and d not in SKIP:
                todo.append(d)
    print("packages (%d):" % len(pkgs), " ".join(pkgs))
    for p in pkgs:
        r = subprocess.run(["apt-get", "download", p], cwd=CACHEDIR, capture_output=True, text=True)
        if r.returncode != 0:
            print("DOWNLOAD-FAIL", p, r.stderr.strip()[:120])
        else:
            print("got", p)
    debdir = PREFIX + "/__debroot"
    for f in sorted(os.listdir(CACHEDIR)):
        if not f.endswith(".deb"):
            continue
        r = subprocess.run(["dpkg-deb", "-x", os.path.join(CACHEDIR, f), debdir])
        if r.returncode != 0:
            print("EXTRACT-FAIL", f)
    print("extracted to", debdir)

if __name__ == "__main__":
    main()
