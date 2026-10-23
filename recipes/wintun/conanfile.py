from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
from conan.errors import ConanInvalidConfiguration

import os

class PackageConan(ConanFile):
    name = "wintun"
    version = "0.14.1"
    settings = "os", "arch"

    @property
    def _arch(self):
        return {
            "x86_64": "amd64",
            "armv8": "arm64",
            "x86": "x86"
        }.get(str(self.settings.arch))

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} is supposed to be run on Windows only"
            )
        
        if not self._arch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.arch}"
            )

    def source(self):
        get(self, f"https://www.wintun.net/builds/wintun-{self.version}.zip",
            sha256="07c256185d6ee3652e09fa55c0b673e2624b565e02c4b9091c79ca7d2f24ef51", strip_root=True)

    def package(self):
        copy(self, "wintun.dll", src=os.path.join(self.source_folder, "bin", self._arch), dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, "bin", "wintun.dll")
        self.cpp_info.set_property("cmake_target_name", "zx2c4::wintun")
