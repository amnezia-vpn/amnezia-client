#!/usr/bin/env python3
"""CGO compiler shim: clang-cl if present, otherwise MSVC cl with GCC-flag filtering."""

from __future__ import annotations

import os
import subprocess
import sys


def _vs_install() -> str | None:
    pf = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = os.path.join(pf, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(vswhere):
        return None
    r = subprocess.run(
        [vswhere, "-latest", "-products", "*", "-property", "installationPath"],
        capture_output=True,
        text=True,
        check=False,
    )
    if r.returncode != 0 or not r.stdout.strip():
        return None
    return r.stdout.strip().splitlines()[0]


def _find_clang_cl() -> str | None:
    install = _vs_install()
    if not install:
        return None
    llvm = os.path.join(install, "VC", "Tools", "Llvm")
    if not os.path.isdir(llvm):
        return None
    preferred: list[str] = []
    fallback: list[str] = []
    for root, _, files in os.walk(llvm):
        if "clang-cl.exe" not in files:
            continue
        path = os.path.join(root, "clang-cl.exe")
        if "arm64" in root.lower():
            preferred.append(path)
        else:
            fallback.append(path)
    if preferred:
        return preferred[0]
    if fallback:
        return fallback[0]
    direct = os.path.join(llvm, "bin", "clang-cl.exe")
    return direct if os.path.isfile(direct) else None


def _gcc_args_to_msvc(args: list[str]) -> list[str]:
    out: list[str] = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "-x" and i + 1 < len(args):
            i += 2
            continue
        if a == "-o" and i + 1 < len(args):
            obj = args[i + 1]
            if obj.endswith((".o", ".obj")):
                out.append("/Fo" + obj)
            else:
                out.append("/Fe" + obj)
            i += 2
            continue
        if a.startswith("-W") or a.startswith("-f") or a.startswith("-d"):
            i += 1
            continue
        if a.startswith("-m") or a.startswith("-std=") or a in ("-pthread",):
            i += 1
            continue
        if a == "-O2":
            out.append("/O2")
            i += 1
            continue
        if a == "-O0":
            out.append("/Od")
            i += 1
            continue
        if a == "-g":
            out.append("/Zi")
            i += 1
            continue
        if a == "-c":
            out.append("/c")
            i += 1
            continue
        if a.startswith("-D"):
            out.append("/D" + a[2:])
            i += 1
            continue
        if a.startswith("-I"):
            out.append("/I" + a[2:])
            i += 1
            continue
        if not a.startswith("-"):
            out.append(a)
        i += 1
    return out


def main() -> int:
    args = sys.argv[1:]
    clang = _find_clang_cl()
    if clang:
        return subprocess.call([clang, *args])

    msvc = _gcc_args_to_msvc(args)
    return subprocess.call(["cl", *msvc])


if __name__ == "__main__":
    sys.exit(main())
