from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import download, copy
from conan.errors import ConanInvalidConfiguration

import os

class WinSplitTunnel(ConanFile):
    name = "win-split-tunnel"
    version = "1.2.5.0"
    settings = "os", "arch"

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

    def build(self):
        url = f"https://raw.githubusercontent.com/mullvad/mullvadvpn-app-binaries/ff0e3746c89a04314377cffeb52faaa976413a69/{self._target}/split-tunnel"

        files = [
            ("mullvad-split-tunnel.cat", "9bbd10b95a2cf2226b266a52077300c280f7782def69ebbeb892bb60505d9a5f"),
            ("mullvad-split-tunnel.inf", "4ec3c2bdefc2a1df9e4e19cd4301b5774624ea07e61add588067b16f8925f5d7"),
            ("mullvad-split-tunnel.pdb", "ee0e246b18a3bfecfc27104b40f9492ffd2b735582870fbd572f93d55e8e9eaa"),
            ("mullvad-split-tunnel.sys", "4056b22d08115c1a83bc2cafa17de0bb17db3705eac382de77fd7935eeff7edb"),
        ]

        for name, sha256 in files:
            download(self, f"{url}/{name}", os.path.join("prebuilt", name), sha256=sha256)

    def package(self):
        copy(self, "*", src="prebuilt", dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "mullvad::win-split-tunnel")
        self.cpp_info.set_property("cmake_extra_variables", {
            "WIN_SPLIT_TUNNEL_BIN": os.path.join(self.package_folder, "bin").replace("\\", "/")
        })
