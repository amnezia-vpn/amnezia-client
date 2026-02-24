from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy

import os

class AwgGo(ConanFile):
    name = "awg-go"
    version = "0.2.16"

    settings = "os", "arch"

    _os_map = {
        "Linux": "linux",
        "Macos": "darwin",
        "Windows": "windows"
    }

    _arch_map = {
        "x86": "386",
        "x86_64": "amd64",
        "armv8": "arm64",
    }

    def validate(self):
        os = str(self.settings.os)
        if os not in self._os_map:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {os}"
            )
        
        arch = str(self.settings.arch)
        if arch not in self._arch_map:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {arch}"
            )
        
    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-go/archive/refs/tags/v{self.version}.zip",
            sha256="34da7d4189f215f3930de441548bc2a0c89d54d347a4fb85cb9c715fce6413aa", strip_root=True
        )

    def layout(self):
        basic_layout(self, build_folder=".")

    def build(self):
        os = self._os_map[str(self.settings.os)]
        arch = self._arch_map[str(self.settings.arch)]

        self.run(f"GOOS={os} GOARCH={arch} make")
    
    def package(self):
        copy(self, "amneziawg-go", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-go")
