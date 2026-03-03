from conan import ConanFile
from conan.tools.files import get, copy
from conan.tools.gnu import Autotools, AutotoolsToolchain, AutotoolsDeps
from conan.tools.layout import basic_layout

import os

class Openvpn(ConanFile):
    name = "openvpn"
    version = "2.7.0"
    package_type = "application"

    settings = "os", "build_type", "arch"

    def layout(self):
        basic_layout(self)

    def build_requirements(self):
        self.tool_requires("libtool/2.4.7")
        self.tool_requires("automake/1.16.5")

    def requirements(self):
        self.requires("openssl/[>=1.1.0]", visible=False)
        self.requires("lz4/[>=1.7.1]", visible=False)
        self.requires("lzo/2.10", visible=False)

    def source(self):
        get(self, f"https://github.com/OpenVPN/openvpn/archive/refs/tags/v{self.version}.zip",
            sha256="1a65d8587f932c13d55b1f175ff2e1d61d795d9092788662e888054854d4ee3d", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()
        deps = AutotoolsDeps(self)
        deps.generate()

    def build(self):
        at = Autotools(self)
        at.autoreconf()
        at.configure()
        at.make()

    def package(self):
        copy(self, "openvpn", src=os.path.join(self.build_folder, "src", "openvpn"), dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, "openvpn")
