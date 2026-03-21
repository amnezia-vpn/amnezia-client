from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import download, copy
from conan.errors import ConanInvalidConfiguration

import os

class WinSplitTunnel(ConanFile):
    name = "win-split-tunnel"
    version = "1.2.5.0"
    settings = "os", "build_type", "compiler"

    @property
    def _arch(self):
        return {
            "x86_64": "x86_64",
            "armv8": "aarch64"
        }.get(str(self.settings.arch))

    @property
    def _target(self):
        return f"{self._arch}-pc-windows-msvc"

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} supports only Windows"
            )

    def source(self):
        url = f"https://raw.githubusercontent.com/mullvad/mullvadvpn-app-binaries/ff0e3746c89a04314377cffeb52faaa976413a69/{self._target}/split-tunnel"
        download(self, url, "mullvad-split-tunnel.cat", sha256="")
        download(self, url, "mullvad-split-tunnel.inf", sha256="")
        download(self, url, "mullvad-split-tunnel.pdb", sha256="")
        download(self, url, "mullvad-split-tunnel.sys", sha256="")

    def package(self):
        copy(self, "*", src=self.source_folder, dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "mullvad::win-split-tunnel")
