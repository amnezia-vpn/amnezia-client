from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
from conan.tools.gnu import Autotools, AutotoolsToolchain

import os

class AwgGo(ConanFile):
    name = "awg-go"
    version = "0.2.16"
    package_type = "application"
    settings = "os", "arch"

    @property
    def _goos(self):
        return {
            "Linux": "linux",
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

    def layout(self):
        basic_layout(self, build_folder=".")

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not self._goos or not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-go/archive/refs/tags/v{self.version}.zip",
            sha256="34da7d4189f215f3930de441548bc2a0c89d54d347a4fb85cb9c715fce6413aa", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        env.define("GOOS", self._goos)
        env.define("GOARCH", self._goarch)
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        tc.generate(env)

    def build(self):
        at = Autotools(self)
        at.make()

    def package(self):
        copy(self, "amneziawg-go", src=self.build_folder, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, "amneziawg-go")
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-go")
