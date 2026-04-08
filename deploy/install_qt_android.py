#!/usr/bin/env python3
"""
Direct Qt Android package downloader.

aqt (v3.3.0) does not index Qt 6.8+ Android packages because Qt changed
their CDN layout.  This script fetches Updates.xml directly from Qt's CDN,
finds the required packages, and extracts them – replicating what aqt does
internally but without depending on aqt's version index.

Usage:
    python3 install_qt_android.py <qt_version> <output_dir> <arch> [extra_modules...]

Example:
    python3 install_qt_android.py 6.10.1 /opt/Qt android_arm64_v8a \
        qtremoteobjects qt5compat qtimageformats qtshadertools
"""

import os
import re
import sys
import subprocess
import urllib.request
import urllib.error
import xml.etree.ElementTree as ET
from pathlib import Path

# Qt 6.8+ moved Android packages to a separate all_os CDN path.
# Qt <= 6.7 used a linux_x64-specific path with a different naming scheme.
QT_CDN_ALL_OS = "https://download.qt.io/online/qtsdkrepository/all_os"
QT_CDN_LINUX  = "https://download.qt.io/online/qtsdkrepository/linux_x64"

# URL patterns to try, newest first.
# Substitutions: {all_os}, {linux}, {ver} (nodot, e.g. "6101"), {short_arch} (e.g. "arm64_v8a")
REPO_PATTERNS = [
    # Qt 6.8+: all_os/android/qt6_{ver}/qt6_{ver}_{short_arch}
    "{all_os}/android/qt6_{ver}/qt6_{ver}_{short_arch}",
    # Qt <= 6.7: linux_x64/android/qt6_{ver}_{short_arch}/qt6_{ver}_{short_arch}
    "{linux}/android/qt6_{ver}_{short_arch}/qt6_{ver}_{short_arch}",
    # Fallback flat layouts
    "{all_os}/android/qt6_{ver}_{short_arch}",
    "{linux}/android/qt6_{ver}_{short_arch}",
]

# Packages that are always downloaded (base Qt for Android)
BASE_PACKAGES = {
    "qt_base",
    "qtdeclarative",
    "qttools",
    "qtsvg",
}

# Packages never wanted (too large / not needed for a headless build)
SKIP_PATTERNS = re.compile(r"debug_info|source|examples|doc", re.IGNORECASE)


def fetch(url: str, retries: int = 5, timeout: int = 120) -> bytes:
    for attempt in range(1, retries + 1):
        try:
            print(f"  GET {url}")
            with urllib.request.urlopen(url, timeout=timeout) as resp:
                return resp.read()
        except urllib.error.URLError as exc:
            if attempt == retries:
                raise RuntimeError(f"Failed to fetch {url} after {retries} attempts: {exc}")
            print(f"  Retry {attempt}/{retries}: {exc}")


def download_file(url: str, dest: str, retries: int = 5) -> None:
    for attempt in range(1, retries + 1):
        try:
            urllib.request.urlretrieve(url, dest)
            return
        except Exception as exc:
            if attempt == retries:
                raise RuntimeError(f"Download failed: {url}: {exc}")
            print(f"  Retry {attempt}/{retries}: {exc}")


def extract(archive: str, output_dir: str) -> None:
    result = subprocess.run(
        ["7z", "x", "-y", archive, f"-o{output_dir}"],
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Extraction failed:\n{result.stderr.decode()}")


def find_repo_url(version_nodot: str, arch: str) -> tuple[str, bytes]:
    """Try each REPO_PATTERN until Updates.xml is found. Returns (repo_url, xml_bytes)."""
    # Qt CDN uses short arch names without the "android_" prefix in the path
    short_arch = arch.removeprefix("android_")
    for pattern in REPO_PATTERNS:
        repo_url = pattern.format(
            all_os=QT_CDN_ALL_OS,
            linux=QT_CDN_LINUX,
            ver=version_nodot,
            short_arch=short_arch,
        )
        updates_url = f"{repo_url}/Updates.xml"
        try:
            print(f"  Trying {updates_url}")
            xml_bytes = fetch(updates_url, retries=1)
            print(f"  Found repo at: {repo_url}")
            return repo_url, xml_bytes
        except RuntimeError:
            pass
    raise RuntimeError(
        f"Could not find Qt {version_nodot} Android repo for arch={arch}.\n"
        "Tried patterns:\n" + "\n".join(f"  {p}" for p in REPO_PATTERNS)
    )


def install_qt_android(version: str, output_dir: str, arch: str, extra_modules: list[str]) -> None:
    version_nodot = version.replace(".", "")

    wanted = BASE_PACKAGES | set(extra_modules)

    print(f"\n{'='*60}")
    print(f"Qt {version}  arch={arch}")
    print(f"Probing CDN for repo URL...")
    print(f"{'='*60}")

    repo_url, xml_bytes = find_repo_url(version_nodot, arch)
    root = ET.fromstring(xml_bytes)

    # Archives contain relative paths (include/, lib/, …) so extract into the
    # versioned arch-specific prefix, e.g. /opt/Qt/6.10.1/android_arm64_v8a/
    extract_dir = str(Path(output_dir) / version / arch)
    Path(extract_dir).mkdir(parents=True, exist_ok=True)

    installed = 0
    for pkg in root.findall(".//PackageUpdate"):
        name = pkg.findtext("Name", "")
        pkg_ver = pkg.findtext("Version", "")
        archives_str = pkg.findtext("DownloadableArchives", "") or ""

        if not archives_str:
            continue
        if arch not in name:
            continue
        if SKIP_PATTERNS.search(name):
            continue

        # The base package (e.g. qt.qt6.6101.android_arm64_v8a) bundles qtbase,
        # qtsvg, qtdeclarative and qttools – its name does not contain module keywords.
        # Detect it by checking that the name ends with the arch identifier.
        is_base_package = name.endswith(f".{arch}") and "addons" not in name
        if not is_base_package and not any(mod in name for mod in wanted):
            continue

        for raw_archive in archives_str.split(","):
            archive_name = raw_archive.strip()
            if not archive_name or SKIP_PATTERNS.search(archive_name):
                continue

            url = f"{repo_url}/{name}/{pkg_ver}{archive_name}"
            tmp = f"/tmp/qt_{archive_name}"
            print(f"\n[{name}] {archive_name}")
            download_file(url, tmp)
            extract(tmp, extract_dir)
            os.remove(tmp)
            installed += 1

    if installed == 0:
        raise SystemExit(
            f"No packages found for arch={arch} in {repo_url}/Updates.xml\n"
            "Check Qt version or arch name."
        )

    print(f"\nInstalled {installed} archive(s) for {arch}")

    # Run qtbase's post-install fixup if present
    fixup = Path(extract_dir) / "bin" / "qt-cmake-private-install.cmake"
    if fixup.exists():
        subprocess.run(["cmake", "-P", str(fixup)], check=False)


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)

    qt_version = sys.argv[1]
    out_dir = sys.argv[2]
    target_arch = sys.argv[3]
    modules = sys.argv[4:]

    install_qt_android(qt_version, out_dir, target_arch, modules)
