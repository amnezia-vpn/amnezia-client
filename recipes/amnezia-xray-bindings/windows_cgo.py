"""Windows CGO helpers: llvm-mingw for ARM64, MinGW via Conan for x86_64."""

import os
import re
import subprocess

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


def msvc_machine(arch: str) -> str:
    return {"x86_64": "X64", "armv8": "ARM64", "x86": "X86"}.get(arch, "ARM64")


def go_tool_exe(conanfile) -> str:
    go = conanfile.dependencies.build["go"]
    return os.path.join(go.package_folder, "bin", "go.exe")


def _vs_install_path(conanfile) -> str:
    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = os.path.join(program_files_x86, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(vswhere):
        raise RuntimeError(f"{conanfile}: vswhere.exe not found at {vswhere}")

    completed = subprocess.run(
        [vswhere, "-latest", "-products", "*", "-property", "installationPath"],
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0 or not completed.stdout.strip():
        raise RuntimeError(
            f"{conanfile}: vswhere failed ({completed.returncode}): "
            f"{completed.stderr or completed.stdout}"
        )
    return completed.stdout.strip().splitlines()[0]


def _vcvars_ver_flag(conanfile) -> str:
    version = str(conanfile.settings.get_safe("compiler.version", ""))
    if not version:
        return ""
    minor = {"192": "14.2", "193": "14.3", "194": "14.4", "195": "14.5"}
    ver = minor.get(version)
    return f" -vcvars_ver={ver}" if ver else ""


def _vcvars_arch_arg(target_arch: str) -> str:
    import platform

    host = platform.machine().lower()
    host_is_arm = host in ("arm64", "aarch64")

    if target_arch == "armv8":
        return "arm64" if host_is_arm else "amd64_arm64"
    if target_arch == "x86_64":
        if host_is_arm:
            return "arm64_x64"
        return "amd64" if host in ("amd64", "x86_64") else "x86_amd64"
    return "arm64" if host_is_arm else "amd64"


def msvc_vcvars_cmd(conanfile) -> str:
    target_arch = str(conanfile.settings.arch)
    install_path = _vs_install_path(conanfile)
    vcvarsall = os.path.join(install_path, "VC", "Auxiliary", "Build", "vcvarsall.bat")
    arch_arg = _vcvars_arch_arg(target_arch)
    ver_flag = _vcvars_ver_flag(conanfile)
    return f'call "{vcvarsall}" {arch_arg}{ver_flag}'


def _find_llvm_mingw_toolchain(root_dir: str) -> tuple[str, str]:
    gcc_names = (
        "aarch64-w64-windows-gnu-gcc.exe",
        "aarch64-w64-mingw32-gcc.exe",
    )
    for gcc_name in gcc_names:
        for root, _, files in os.walk(root_dir):
            if gcc_name in files:
                bindir = os.path.join(root, "")
                return os.path.join(root, gcc_name), bindir

    for root, _, files in os.walk(root_dir):
        for fname in files:
            if fname.endswith("-gcc.exe") and "aarch64" in fname:
                return os.path.join(root, fname), root
    raise RuntimeError(f"llvm-mingw gcc not found under {root_dir}")


def ensure_llvm_mingw(conanfile) -> tuple[str, str]:
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

    gcc, bindir = _find_llvm_mingw_toolchain(dest)
    conanfile.output.info(f"CGO compiler: {gcc}")
    return gcc, bindir


def run_llvm_mingw_go_build(
    conanfile,
    src: str,
    out: str,
    goarch: str,
    *,
    goarm=None,
    extra_args="-ldflags=-w -buildmode=c-shared",
) -> None:
    """Build c-shared with llvm-mingw GCC (same approach as other Go Windows/arm64 projects)."""
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


def ensure_msvc_import_lib(
    conanfile,
    build_dir: str,
    dll_name: str,
    base_name: str,
) -> None:
    """MSVC import library for linking the DLL from the main app (MSVC/Qt)."""
    lib_path = os.path.join(build_dir, f"{base_name}.lib")
    if os.path.isfile(lib_path):
        return

    dll_path = os.path.join(build_dir, dll_name)
    if not os.path.isfile(dll_path):
        raise RuntimeError(f"{conanfile}: DLL not found: {dll_path}")

    machine = msvc_machine(str(conanfile.settings.arch))
    def_path = os.path.join(build_dir, f"{base_name}.def")
    exports_txt = os.path.join(build_dir, f"{base_name}.exports.txt")
    vc = msvc_vcvars_cmd(conanfile)

    conanfile.run(f'{vc} && dumpbin /nologo /exports "{dll_path}" > "{exports_txt}"')

    with open(exports_txt, encoding="utf-8", errors="replace") as exports_file:
        text = exports_file.read()

    exports = []
    in_table = False
    for line in text.splitlines():
        if "ordinal hint RVA" in line:
            in_table = True
            continue
        if not in_table:
            continue
        match = re.match(r"\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)", line)
        if match:
            name = match.group(1)
            if name != "[noname]":
                exports.append(name)

    if not exports:
        raise RuntimeError(f"{conanfile}: No exports found in {dll_path}")

    with open(def_path, "w", encoding="ascii") as def_file:
        def_file.write(f"LIBRARY {base_name}\nEXPORTS\n")
        for symbol in exports:
            def_file.write(f"    {symbol}\n")

    conanfile.run(
        f'{vc} && lib /nologo /def:"{def_path}" /out:"{lib_path}" /machine:{machine}',
    )

    try:
        os.remove(exports_txt)
    except OSError:
        pass
