from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir, rename
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import VirtualBuildEnv

import os
import sys

_recipe_dir = os.path.dirname(os.path.abspath(__file__))
if _recipe_dir not in sys.path:
    sys.path.insert(0, _recipe_dir)

from windows_cgo import ensure_msvc_import_lib, is_windows_arm64, run_llvm_mingw_go_build


class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.1.0"
    settings = "os", "arch", "compiler"
    exports = "windows_cgo.py"

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "iOS": "ios",
            "Macos": "darwin",
            "Windows": "windows"
        }.get(str(self.settings.os))

    @property
    def _goarch(self):
        return {
            "x86": "386",
            "x86_64": "amd64",
            "armv8": "arm64"
        }.get(str(self.settings.arch))

    @property
    def _is_windows(self):
        return str(self.settings.os).startswith("Windows")

    @property
    def _windows_arm64(self):
        return is_windows_arm64(self)

    def config_options(self):
        self.package_type = "shared-library" if self._is_windows else "static-library"

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")
        # Keep compiler settings on Windows ARM64 so VCVars can expose cl/link to CGO.
        if self._is_windows and str(self.settings.arch) != "armv8":
            del self.settings.compiler

    def layout(self):
        basic_layout(self)

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self._is_windows and not self._windows_arm64:
            # x86/x64: MinGW gendef + dlltool for import libraries.
            self.tool_requires("mingw-builds/15.1.0")

    def validate(self):
        if not self._goos or not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/v1.1.0.zip",
            sha256="6ea768ec7002cedd422a39aea17704b888acaf794432aa5937cfc92fb6d80eb5", strip_root=True)

    def generate(self):
        # Go + (MinGW or MSVC) on PATH for all Windows/Linux variants.
        VirtualBuildEnv(self).generate()

        if self._is_windows and self._windows_arm64:
            # MSVC CGO: activate vcvars only in build() (see run_msvc_go_build).
            return

        tc = AutotoolsToolchain(self)
        if not self._is_windows:
            tc.make_args = [
                "LIB_ARC=libamnezia_xray.a"
            ]
        env = tc.environment()
        env.define("ARCH", self._goarch)
        env.define("GOARCH", self._goarch)
        env.define("GOOS", self._goos)
        if self._is_windows:
            env.define("OS", "windows")
            env.define("CGO_ENABLED", "1")
            env.define("CGO_LDFLAGS", tc.ldflags)
            env.define("CGO_CFLAGS", tc.cflags)
        else:
            env.define("CGO_LDFLAGS", tc.ldflags)
            env.define("CGO_CFLAGS", tc.cflags)
        tc.generate(env)

    def build(self):
        if self._is_windows:
            self._build_windows_native()
            return
        with chdir(self, self.source_folder):
            autotools = Autotools(self)
            autotools.make()

    @property
    def _windows_artifact_dir(self):
        """Keep Windows outputs under build_folder (not source_folder/llvm-mingw tree)."""
        return os.path.join(self.build_folder, "build")

    def _build_windows_native(self):
        """
        Windows: c-shared DLL. x86_64 uses MinGW gendef + dlltool; ARM64 uses llvm-mingw CGO.
        """
        src = self.source_folder
        bdir = self._windows_artifact_dir
        os.makedirs(bdir, exist_ok=True)
        dll_name = "amnezia_xray.dll"
        dll_path = os.path.join(bdir, dll_name)

        if self._windows_arm64:
            run_llvm_mingw_go_build(self, src, dll_path, self._goarch)
            ensure_msvc_import_lib(self, bdir, dll_name, "amnezia_xray")
        else:
            self.run(
                f'cd /d "{src}" && go build -ldflags=-w -o "{dll_path}" -buildmode=c-shared .',
                env="conanbuild",
            )
        if not self._windows_arm64:
            self.run(
                f'cd /d "{bdir}" && gendef {dll_name}',
                env="conanbuild",
            )
            self.run(
                f'cd /d "{bdir}" && dlltool -d amnezia_xray.def -l amnezia_xray.lib -D {dll_name}',
                env="conanbuild",
            )

        def_path = os.path.join(bdir, "amnezia_xray.def")
        try:
            if os.path.isfile(def_path):
                os.remove(def_path)
        except OSError:
            pass

    def _rename_header(self):
        if not self._is_windows:
            rename(self, os.path.join(self.package_folder, "include", "libamnezia_xray.h"),
                   os.path.join(self.package_folder, "include", "amnezia_xray.h"))

    def _rename_libs(self):
        lib_dir = os.path.join(self.package_folder, "lib")
        for fname in os.listdir(lib_dir):
            if not fname.startswith("lib"):
                src = os.path.join(lib_dir, fname)
                dst = os.path.join(lib_dir, "lib" + fname)
                os.rename(src, dst)

    def package(self):
        if self._is_windows:
            art = self._windows_artifact_dir
        else:
            art = os.path.join(self.build_folder, "build")
            if not os.path.isdir(art):
                art = self.build_folder
        copy(self, "*.h", src=art, dst=os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "*.a", src=art, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.lib", src=art, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dll", src=art, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        self._rename_header()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        if self._is_windows:
            self.cpp_info.libs = ["amnezia_xray"]
            self.cpp_info.libdirs = ["lib"]
            self.cpp_info.bindirs = ["bin"]
        else:
            self.cpp_info.libs = collect_libs(self)
