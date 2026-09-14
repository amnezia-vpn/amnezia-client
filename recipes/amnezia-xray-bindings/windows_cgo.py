"""Windows ARM64 CGO helpers: llvm-mingw toolchain + MSVC-compatible import library."""

import os

# https://github.com/mstorsjo/llvm-mingw/releases
LLVM_MINGW_VERSION = "20260519"
LLVM_MINGW_URL = (
    f"https://github.com/mstorsjo/llvm-mingw/releases/download/"
    f"{LLVM_MINGW_VERSION}/llvm-mingw-{LLVM_MINGW_VERSION}-ucrt-aarch64.zip"
)
LLVM_MINGW_SHA256 = "7bb5e3c9964dc29555cababb55e69eea02a51e98e1240eb9513e6540c7bae0ea"


def is_windows_arm64(conanfile) -> bool:
    return (
        str(conanfile.settings.get_safe("os", "")).startswith("Windows")
        and str(conanfile.settings.arch) == "armv8"
    )


def go_tool_exe(conanfile) -> str:
    go = conanfile.dependencies.build["go"]
    return os.path.join(go.package_folder, "bin", "go.exe")


def _find_tool(bindir: str, names) -> str:
    for name in names:
        path = os.path.join(bindir, name)
        if os.path.isfile(path):
            return path
    raise RuntimeError(f"none of {names} found in {bindir}")


def _find_llvm_mingw_toolchain(root_dir: str):
    """Returns (aarch64 gcc.exe, its bin dir)."""
    for gcc_name in ("aarch64-w64-windows-gnu-gcc.exe", "aarch64-w64-mingw32-gcc.exe"):
        for root, _, files in os.walk(root_dir):
            if gcc_name in files:
                return os.path.join(root, gcc_name), root
    raise RuntimeError(f"llvm-mingw gcc not found under {root_dir}")


def ensure_llvm_mingw(conanfile):
    """Download and extract llvm-mingw (UCRT, aarch64). Returns (gcc.exe, bin_dir)."""
    from conan.tools.files import download, unzip

    dest = os.path.join(conanfile.build_folder, "llvm-mingw")
    ready = os.path.join(dest, ".extracted")
    if not os.path.isfile(ready):
        zip_path = os.path.join(conanfile.build_folder, "llvm-mingw.zip")
        conanfile.output.info(f"Downloading llvm-mingw {LLVM_MINGW_VERSION} for Windows ARM64 CGO")
        download(conanfile, LLVM_MINGW_URL, zip_path, sha256=LLVM_MINGW_SHA256)
        unzip(conanfile, zip_path, dest)
        with open(ready, "w", encoding="ascii") as marker:
            marker.write(LLVM_MINGW_VERSION)

    return _find_llvm_mingw_toolchain(dest)


def run_llvm_mingw_go_build(
    conanfile,
    src: str,
    out: str,
    goarch: str,
    *,
    goarm=None,
    extra_args="-ldflags=-w -buildmode=c-shared",
) -> None:
    """Build with llvm-mingw driving CGO (mingw-builds has no Windows ARM64 binaries)."""
    gcc, bindir = ensure_llvm_mingw(conanfile)
    go = go_tool_exe(conanfile)
    goarm_set = f"&& set GOARM={goarm}" if goarm else ""
    bindir_q = bindir.replace('"', '""')
    gcc_q = gcc.replace('"', '""')
    conanfile.run(
        f'set "PATH={bindir_q};%PATH%"&& set CGO_ENABLED=1&& set GOOS=windows&& set GOARCH={goarch}{goarm_set}&& '
        f'set "CC={gcc_q}"&& set "CXX={gcc_q}"&& '
        f'"{go}" build -C "{src}" {extra_args} -o "{out}" .',
    )


def ensure_import_lib(conanfile, build_dir: str, base_name: str, machine="arm64") -> None:
    """Import library so the MSVC-built app can link the DLL.

    The module definition file is produced by the linker itself during the Go build
    (-Wl,--output-def), so no MSVC tooling (vswhere/dumpbin/lib) is needed here.
    """
    _, bindir = ensure_llvm_mingw(conanfile)
    dlltool = _find_tool(bindir, ("llvm-dlltool.exe", "aarch64-w64-mingw32-dlltool.exe"))
    def_path = os.path.join(build_dir, f"{base_name}.def")
    lib_path = os.path.join(build_dir, f"{base_name}.lib")
    if not os.path.isfile(def_path):
        raise RuntimeError(f"{conanfile}: module definition file not found: {def_path}")
    conanfile.run(f'"{dlltool}" -m {machine} -D {base_name}.dll -d "{def_path}" -l "{lib_path}"')
