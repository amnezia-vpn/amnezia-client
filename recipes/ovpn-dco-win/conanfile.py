from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
from conan.errors import ConanInvalidConfiguration

import os


class OvpnDcoWin(ConanFile):
    name = "ovpn-dco-win"
    version = "2.8.3"
    settings = "os", "arch"

    # Microsoft-attestation-signed driver binaries; they MUST be shipped
    # byte-identical — rebuilding or re-signing breaks the signature chain.
    # The zip carries win10/ (NetAdapterCx 2.0) and win11/ (NetAdapterCx 2.1)
    # flavours of ovpn-dco.{inf,cat,sys}; the service picks one at install
    # time based on the OS build.
    _arch_map = {
        "x86": "x86",
        "x86_64": "amd64",
        "armv8": "arm64",
    }

    _sha256 = {
        "amd64": "aab2b875a2c8e8bf3a262a7c4e496256a7dfc995f12f568f7e70f372ff118497",
        "arm64": "a0df0d9de5cfde8a7a862da8dd418955c15f4ced0a079d4601a331b9d69243bb",
    }

    @property
    def _arch(self):
        return self._arch_map.get(str(self.settings.arch))

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} supports only Windows"
            )
        if self._arch not in self._sha256:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.arch} architecture"
            )

    def build(self):
        get(self,
            f"https://github.com/OpenVPN/ovpn-dco-win/releases/download/{self.version}/ovpn-dco-win-{self.version}-{self._arch}.zip",
            sha256=self._sha256[self._arch])

    def package(self):
        for flavour in ("win10", "win11"):
            copy(self, "*", src=os.path.join(self.build_folder, flavour),
                 dst=os.path.join(self.package_folder, "bin", flavour))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "openvpn::ovpn-dco-win")
        self.cpp_info.set_property("cmake_extra_variables", {
            "OVPN_DCO_WIN_BIN": os.path.join(self.package_folder, "bin").replace("\\", "/")
        })
