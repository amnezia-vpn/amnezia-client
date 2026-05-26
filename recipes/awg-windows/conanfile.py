from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import get, copy, chdir
from conan.tools.gnu import AutotoolsToolchain
from conan.tools.env import Environment, VirtualBuildEnv

import os
import sys

_recipe_dir = os.path.dirname(os.path.abspath(__file__))
if _recipe_dir not in sys.path:
    sys.path.insert(0, _recipe_dir)

from windows_cgo import is_windows_arm64, run_llvm_mingw_go_build


class AwgWindows(ConanFile):
    name = "awg-windows"
    version = "0.1.8"
    settings = "os", "arch", "compiler", "build_type"
    exports = "windows_cgo.py"

    @property
    def _goarm(self):
        return {
            "armv5el": "5",
            "armv5hf": "5",
            "armv6": "6",
            "armv7": "7",
            "armv7hf": "7",
            "armv7s": "7",
            "armv7k": "7",
        }.get(str(self.settings.arch))

    @property
    def _goarch(self):
        return {
            "x86": "386",
            "x86_64": "amd64",
            "armv5el": "arm",
            "armv5hf": "arm",
            "armv6": "arm",
            "armv7": "arm",
            "armv7hf": "arm",
            "armv7s": "arm",
            "armv7k": "arm",
            "armv8": "arm64",
            "armv8_32": "arm64",
            "armv8.3": "arm64",
            "arm64ec": "arm64"
        }.get(str(self.settings.arch))

    @property
    def _windows_arm64(self):
        return is_windows_arm64(self)

    def layout(self):
        basic_layout(self)

    def configure(self):
        if not self._windows_arm64:
            self.settings.rm_safe("compiler")
            self.settings.rm_safe("build_type")

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} is to be used on Windows only!"
            )
        if not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.arch} architecture"
            )

    def build_requirements(self):
        if not self._windows_arm64:
            self.tool_requires("mingw-builds/15.1.0")
        self.tool_requires("go/1.26.0")

    def requirements(self):
        self.requires("wintun/[*]")

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-windows/archive/refs/tags/v{self.version}.zip",
            sha256="1de472832b332515c96cdf14ea887edde42ed7ad173675280c51baa9a3ef62f2", strip_root=True)

    def generate(self):
        VirtualBuildEnv(self).generate()
        if self._windows_arm64:
            return

        env = Environment()
        env.define("GOOS", "windows")
        if self._goarm:
            env.define("GOARM", self._goarm)
        env.define("GOARCH", self._goarch)
        env.define("CGO_ENABLED", "1")

        tc = AutotoolsToolchain(self)
        tc.extra_cflags = [
            "-Wall",
            "-Wno-unused-function",
            "-Wno-switch",
            "-DWINVER=0x0601"
        ]
        tc.extra_ldflags = [
            "-Wl,--dynamicbase",
            "-Wl,--nxcompat",
            "-Wl,--export-all-symbols",
            "-Wl,--high-entropy-va"
        ]
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)

        env.vars(self, scope="build").save_script("conanbuild")

    def build(self):
        out_dll = os.path.join(self.build_folder, "tunnel.dll")
        if self._windows_arm64:
            run_llvm_mingw_go_build(
                self,
                self.source_folder,
                out_dll,
                self._goarch,
                goarm=self._goarm,
                extra_args='-buildmode c-shared -ldflags="-w -s" -trimpath -v',
            )
            return

        with chdir(self, self.source_folder):
            self.run(
                f'go build -buildmode c-shared -ldflags="-w -s" -trimpath -v '
                f'-o "{out_dll}"',
                env="conanbuild",
            )

    def package(self):
        copy(self, "tunnel.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, "bin", "tunnel.dll")
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-windows")
