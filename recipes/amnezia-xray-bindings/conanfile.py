# conanfile.py
from conan import ConanFile
from conan.tools.files import get, copy, collect_libs
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration

import os

class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.1.0"

    settings = "os", "arch"

    _os_map = {
        "Linux": "linux",
        "iOS": "ios",
        "Macos": "macos",
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

    def layout(self):
        basic_layout(self, build_folder=os.path.join(self.folders.source, "build"))

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self.settings.os != "Windows":
            self.build_requires("make/4.4.1")

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/v1.1.0.zip",
            sha256="6ea768ec7002cedd422a39aea17704b888acaf794432aa5937cfc92fb6d80eb5", strip_root=True)

    def build(self):
        os = self._os_map.get(str(self.settings.os))
        arch = self._arch_map.get(str(self.settings.arch))

        self.run(f"ARCH={arch} OS={os} make -C {self.source_folder}")

    def package(self):        
        copy(self, "*.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))

        # workaround of bad naming strategy in amnezia-xray-bindings
        # TODO: change it and kick out the code below
        lib_dir = os.path.join(self.package_folder, "lib")
        for fname in os.listdir(lib_dir):
            if not fname.startswith("lib"):
                src = os.path.join(lib_dir, fname)
                dst = os.path.join(lib_dir, "lib" + fname)
                os.rename(src, dst)

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        self.cpp_info.libs = collect_libs(self)
