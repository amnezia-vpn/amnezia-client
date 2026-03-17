# conanfile.py
from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import Environment

import os

class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.1.0"

    settings = "os", "arch"

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

    def validate(self):
        if not self._goos or not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def layout(self):
        basic_layout(self)

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/v1.1.0.zip",
            sha256="6ea768ec7002cedd422a39aea17704b888acaf794432aa5937cfc92fb6d80eb5", strip_root=True)

    def generate(self):
        tc = AutotoolsToolchain(self)
        env = Environment()
        env.define("ARCH", self._goarch)
        env.define("GOARCH", self._goarch)
        env.define("GOOS", self._goos)
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        if self._is_windows:
            env.define("OS", "windows")
        tc.generate(env)

    def build(self):
        with chdir(self, self.source_folder):
            autotools = Autotools(self)
            autotools.make()

    def _rename_libs(self):
        # workaround of bad naming strategy in amnezia-xray-bindings
        # TODO: change it and kick out the code below
        lib_dir = os.path.join(self.package_folder, "lib")
        for fname in os.listdir(lib_dir):
            if not fname.startswith("lib"):
                src = os.path.join(lib_dir, fname)
                dst = os.path.join(lib_dir, "lib" + fname)
                os.rename(src, dst)

    def package(self):
        copy(self, "*.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        self._rename_libs()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        self.cpp_info.libs = collect_libs(self)
