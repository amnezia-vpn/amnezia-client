from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, collect_libs

import os

class AwgApple(ConanFile):
    name = "awg-apple"
    version = "2.0.1"

    settings = "os", "arch"

    _os_map = {
        "iOS": "iphoneos",
        "Macos": "macosx"
    }

    _arch_map = {
        "x86_64": "x86_64",
        "armv8": "arm64"
    }

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self.settings.os != "Windows":
            self.build_requires("make/4.4.1")

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

    def layout(self):
        basic_layout(self, build_folder=os.path.join(self.folders.source, "Sources/WireGuardKitGo"))

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-apple/archive/refs/tags/v{self.version}.zip",
            sha256="9fe4f8cfbb6a751558b54b7979db3a5ea46e49731912aae99f093e84a1433e97", strip_root=True
        )

    def build(self):
        os = self._os_map.get(str(self.settings.os))
        arch = self._arch_map.get(str(self.settings.arch))

        self.run(f"ARCHS={arch} PLATFORM_NAME={os} make")

    def package(self):
        copy(self, "wireguard.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-apple")
        self.cpp_info.libs = collect_libs(self)
