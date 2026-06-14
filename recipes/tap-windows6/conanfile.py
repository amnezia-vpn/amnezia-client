from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import get, copy

import os

class TapWindows6(ConanFile):
    name = "tap-windows6"
    version = "9.27.0"
    settings = "os", "arch"

    @property
    def _arch(self):
        return {
            "x86": "i386",
            "x86_64": "amd64",
            "armv8": "arm64"
        }.get(str(self.settings.arch))

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does only support Windows"
            )

        if not self._arch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.arch}"
            )

    def source(self):
        get(self, f"https://github.com/OpenVPN/tap-windows6/releases/download/{self.version}/dist.win10.zip",
            sha256="36e2609b7ceefedcb978ce5c48caf9e0e5af83423717c4e2e3c1d7ebca8f62a5", strip_root=True)

    def package(self):
        copy(self, "*.h", src=os.path.join(self.source_folder, "include"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "*", src=os.path.join(self.source_folder, self._arch), dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "openvpn::tap-windows6")
        self.cpp_info.set_property("cmake_extra_variables", {
            "TAP_WINDOWS6_BIN": os.path.join(self.package_folder, "bin").replace("\\", "/")
        })
