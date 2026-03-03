from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
from conan.errors import ConanInvalidConfiguration

import os

class Tun2Socks(ConanFile):
    name = "tun2socks"
    version = "2.6.0"

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

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self.settings.os == "Windows":
            self.tool_requires("mingw-builds/15.1.0")
        else:
            self.build_requires("make/4.4.1")

    def source(self):
        #TODO: get tunnel.dll for windows
        get(self, f"https://github.com/xjasonlyu/tun2socks/archive/refs/tags/v{self.version}.zip",
            sha256="a7ef9cec1c30dfe9971af89a8aac767fd3d2a4df833e92b635642c2f0204c701", strip_root=True
        )
        

    def configure(self):
        self._goos = self._os_map.get(str(self.settings.os))
        if not self._goos:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self._goos}"
            )

        self._goarch = self._arch_map.get(str(self.settings.arch))
        if not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self._goarch}"
            )
        
        self._ext = ".exe" if self.settings.os == "Windows" else ""

    def layout(self):
        basic_layout(self, build_folder=".")

    def build(self):
        self.run(f"make {self._goos}-{self._goarch}")

    def package(self):
        src_name = f"tun2socks-{self._goos}-{self._goarch}{self._ext}"
        dst_name = f"tun2socks{self._ext}"

        copy(self, src_name, src=os.path.join(self.build_folder, "build"), dst=self.package_folder)
        os.rename(
            os.path.join(self.package_folder, src_name),
            os.path.join(self.package_folder, dst_name)
        )

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, f"tun2socks{self._ext}")

        self.cpp_info.set_property("cmake_target_name", "xjasonlyu::tun2socks")
