from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, collect_libs
from conan.tools.apple import is_apple_os
from conan.tools.gnu import AutotoolsToolchain, Autotools

import os

class AwgApple(ConanFile):
    name = "awg-apple"
    version = "2.0.1"
    settings = "os", "arch", "compiler"

    @property
    def _goarch(self):
        arch_map = {
            "armv8": "arm64",
            "x86_64": "x86_64",
        }
        archs = str(self.settings.arch).split("|")
        return " ".join(arch_map.get(arch, arch) for arch in archs)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        basic_layout(self, build_folder=os.path.join(self.folders.source, "Sources/WireGuardKitGo"))

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-apple/archive/refs/tags/v{self.version}.zip",
            sha256="9fe4f8cfbb6a751558b54b7979db3a5ea46e49731912aae99f093e84a1433e97", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        sdk = self.settings.get_safe("os.sdk", "macosx")
        tc.make_args = [
            f"ARCHS={self._goarch}",
            f"PLATFORM_NAME={sdk}"
        ]
        tc.generate()

    def build(self):
        autotools = Autotools(self)
        autotools.make()
        autotools.make("version-header")

    def package(self):
        copy(self, "wireguard.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.h", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-apple")
        self.cpp_info.libs = collect_libs(self)
