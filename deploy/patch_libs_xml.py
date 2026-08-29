#!/usr/bin/env python3
"""Add every packaged native library to androiddeployqt's libs.xml preload list.

Since Android 10 the bionic linker namespace blocks dlopen() by filesystem path
for libraries that were not preloaded through System.loadLibrary. Qt loads its
QML plugins by path, so any plugin missing from res/values/libs.xml fails at
runtime with "dlopen failed: library ... not found". androiddeployqt lists only
the libraries it knows about, so the rest are injected here.

Usage:
    patch_libs_xml.py [ANDROID_BUILD_DIR]

ANDROID_BUILD_DIR defaults to deploy/build/client/android-build next to this
script.
"""

import os
import re
import sys

# Loaded by the runtime itself, not through the preload list.
EXCLUDED_PREFIXES = ("AmneziaVPN", "plugins_platforms_qtforandroid")


def default_build_dir():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(repo_root, "deploy", "build", "client", "android-build")


def library_names(libs_dir):
    """Return the loadable library names (without lib/.so) found in libs_dir."""
    names = []
    for entry in os.listdir(libs_dir):
        if not entry.startswith("lib") or not entry.endswith(".so"):
            continue
        name = entry[len("lib"):-len(".so")]
        if name.startswith(EXCLUDED_PREFIXES):
            continue
        names.append(name)
    return sorted(names)


def main():
    build_dir = sys.argv[1] if len(sys.argv) > 1 else default_build_dir()
    libs_root = os.path.join(build_dir, "libs")
    libs_xml_path = os.path.join(build_dir, "res", "values", "libs.xml")

    if not os.path.isfile(libs_xml_path):
        print("libs.xml not found at {}".format(libs_xml_path), file=sys.stderr)
        return 1
    if not os.path.isdir(libs_root):
        print("no libs directory at {}".format(libs_root), file=sys.stderr)
        return 1

    with open(libs_xml_path, "r", encoding="utf-8") as f:
        content = f.read()

    items = []
    for abi in sorted(os.listdir(libs_root)):
        abi_dir = os.path.join(libs_root, abi)
        if not os.path.isdir(abi_dir):
            continue
        for name in library_names(abi_dir):
            entry = "{};{}".format(abi, name)
            if "<item>{}</item>".format(entry) not in content:
                items.append("        <item>{}</item>".format(entry))

    if not items:
        print("All libraries are already present in libs.xml")
        return 0

    # Append to the load_local_libs array, which is what Qt preloads.
    marker = re.search(r"</array>\s*\n\s*<array name=\"load_local_libs\">", content)
    if marker is None:
        print("load_local_libs array not found in {}".format(libs_xml_path), file=sys.stderr)
        return 1

    print("Injecting {} missing libraries into libs.xml".format(len(items)))
    replacement = "\n" + "\n".join(items) + "\n    " + marker.group(0)
    content = content[:marker.start()] + replacement + content[marker.end():]

    with open(libs_xml_path, "w", encoding="utf-8") as f:
        f.write(content)

    print("libs.xml patched")
    return 0


if __name__ == "__main__":
    sys.exit(main())
