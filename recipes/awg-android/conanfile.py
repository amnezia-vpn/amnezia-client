from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, load, replace_in_file, rmdir, save
from conan.tools.env import VirtualBuildEnv
from conan.errors import ConanException, ConanInvalidConfiguration
from conan.tools.scm import Git

import os
import platform
import re
import shutil
from io import StringIO
from pathlib import Path

class AwgAndroid(ConanFile):
    name = "awg-android"
    version = "3.1.20260814"
    settings = "os", "arch", "build_type", "compiler"

    _WINDOWS_GO_HOST_PLATFORM = "windows-amd64"

    def export_sources(self):
        export_conandata_patches(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.4.1 <4]")
        if platform.system() == "Windows":
            self.tool_requires("ninja/[*]")
            self.tool_requires("go/1.26.0")
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} v{self.version} does not support {self.settings.os}")

    def source(self):
        if os.path.isdir(self.source_folder):
            for entry in os.listdir(self.source_folder):
                path = os.path.join(self.source_folder, entry)
                shutil.rmtree(path) if os.path.isdir(path) else os.remove(path)
        git = Git(self)
        git.clone(
            url="https://github.com/amnezia-vpn/amneziawg-android.git",
            target=".",
            args=["--recurse-submodules", "--branch", f"v{self.version}"]
        )

    def generate(self):
        venv = VirtualBuildEnv(self)
        if platform.system() == "Windows":
            venv.environment().define("GOTOOLCHAIN", "local")
        # sum.golang.org is sometimes unreachable from CI runners entirely
        # ("connection refused"); go.sum already pins the exact hashes we
        # need, so the extra public-transparency-log check isn't required.
        venv.environment().define("GOSUMDB", "off")
        venv.generate()

        tc = CMakeToolchain(self)
        tc.variables["GRADLE_USER_HOME"] = Path(os.path.join(self.build_folder, "gradle_user_home")).as_posix()
        tc.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = Path(os.path.join(self.build_folder, "out")).as_posix()
        # not to warn in case of strtok() usage
        tc.extra_cflags = ["-Wno-deprecated-declarations"]
        tc.generate()

    def _patch_sources(self):
        if platform.system() == 'Darwin':
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'flock "$@.lock" -c \' \\\n',
                "",
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'mv "$@.tmp" "$@"\'',
                'mv "$@.tmp" "$@"',
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'touch "$@"\'',
                'touch "$@"',
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'sha256sum -c',
                'shasum -a 256 -c'
            )
        elif platform.system() == 'Windows':
            for rel_path in (
                os.path.join("tunnel", "tools", "CMakeLists.txt"),
                os.path.join("tunnel", "tools", "libwg-go", "Makefile"),
            ):
                abs_path = os.path.join(self.source_folder, rel_path)
                content = load(self, abs_path)
                if "\r\n" in content:
                    save(self, abs_path, content.replace("\r\n", "\n"))

            apply_conandata_patches(self)
            self._prepare_patched_go_for_windows()

    def _prepare_patched_go_for_windows(self):
        makefile_path = os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile")
        makefile_content = load(self, makefile_path)
        version_match = re.search(r"^GO_VERSION\s*:=\s*(\S+)", makefile_content, re.MULTILINE)
        if not version_match:
            raise ConanException("Could not find GO_VERSION in libwg-go/Makefile; "
                                  "upstream may have restructured it, patch needs updating")
        go_version = version_match.group(1)

        build_dir = os.path.join(self.build_folder, "generated-src")
        go_dir = os.path.join(build_dir, f"go-{go_version}")
        prepared_marker = os.path.join(go_dir, ".prepared")

        if os.path.exists(prepared_marker):
            return
        gradle_user_home = os.path.join(self.build_folder, "gradle_user_home")
        tarball_path = os.path.join(gradle_user_home, "caches", "golang",
                                     f"go{go_version}.{self._WINDOWS_GO_HOST_PLATFORM}.tar.gz")
        os.makedirs(os.path.dirname(tarball_path), exist_ok=True)
        save(self, tarball_path, "")

        goroot_output = StringIO()
        self.run("go env GOROOT", env="conanbuild", stdout=goroot_output)
        goroot = goroot_output.getvalue().strip()

        os.makedirs(build_dir, exist_ok=True)
        if os.path.isdir(go_dir):
            rmdir(self, go_dir)
        shutil.copytree(goroot, go_dir)

        diff_path = os.path.join(self.source_folder, "tunnel", "tools", "libwg-go",
                                  "goruntime-boottime-over-monotonic.diff")
        diff_content = load(self, diff_path)
        android_archs = ("sys_linux_386.s", "sys_linux_amd64.s", "sys_linux_arm.s", "sys_linux_arm64.s")
        sections = re.split(r"(?=^diff --git )", diff_content, flags=re.MULTILINE)
        filtered_diff = "".join(s for s in sections if any(f"/{name} " in s.splitlines()[0] for name in android_archs) if s.startswith("diff --git"))

        self._apply_runtime_patch_as_line_replacements(filtered_diff, go_dir)

        save(self, prepared_marker, "")

    def _apply_runtime_patch_as_line_replacements(self, diff_content, source_root):
        sections = re.split(r"(?=^diff --git )", diff_content, flags=re.MULTILINE)
        for section in sections:
            if not section.startswith("diff --git "):
                continue
            rel_path = re.search(r"^\+\+\+ b/(\S+)", section, re.MULTILINE).group(1)
            abs_path = os.path.join(source_root, rel_path)
            content = load(self, abs_path)
            lines = section.splitlines()
            i = 0
            while i < len(lines) - 1:
                removed, added = lines[i], lines[i + 1]
                if (removed.startswith("-") and not removed.startswith("---")
                        and added.startswith("+") and not added.startswith("+++")):
                    old, new = removed[1:], added[1:]
                    if new in content:
                        pass
                    elif old in content:
                        content = content.replace(old, new, 1)
                    else:
                        raise ConanException(
                            f"goruntime-boottime-over-monotonic.diff: neither old nor new form of a "
                            f"hunk line found in {rel_path}; upstream Go has changed this code, patch "
                            f"needs updating\n  old: {old!r}\n  new: {new!r}"
                        )
                    i += 2
                else:
                    i += 1
            save(self, abs_path, content)

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure(build_script_folder=os.path.join(self.source_folder, "tunnel", "tools"))
        cmake.build(target=["libwg-go.so", "libwg.so", "libwg-quick.so"])

    def package(self):
        copy(self, "libwg-go.h", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "libwg-go.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))
        copy(self, "libwg.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "bin"))
        copy(self, "libwg-quick.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-android")
        self.cpp_info.libs = [ "wg-go" ]
        self.cpp_info.set_property("cmake_extra_variables", {
            "AMNEZIA_ANDROID_LIBWG_PATH": Path(os.path.join(self.package_folder, "bin", "libwg.so")).as_posix(),
            "AMNEZIA_ANDROID_LIBWG_QUICK_PATH": Path(os.path.join(self.package_folder, "bin", "libwg-quick.so")).as_posix(),
        })
