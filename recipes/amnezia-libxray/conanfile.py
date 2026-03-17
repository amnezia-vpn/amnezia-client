from conan import ConanFile
from conan.tools.files import get, copy
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration

import os
import stat

class AmneziaLibxray(ConanFile):
    name = "amnezia-libxray"
    version = "1.0.0"

    settings = "os", "arch"
    options = {"page_16k": [True, False]}
    default_options = {"page_16k": True}

    def layout(self):
        basic_layout(self, build_folder=".")

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
    
    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} v{self.version} does not support {self.settings.os}")

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-libxray/archive/refs/tags/v1.0.0.zip",
            sha256="0c50c5acd5063a9fc3cfbb5b3e11481d30cfa3762b3cb1d72130248ff498e9df", strip_root=True
        )

    def _patch_sources(self):
        build_path = os.path.join(self.build_folder, "build.sh")
        build_stat = os.stat(build_path)
        os.chmod(build_path, build_stat.st_mode | stat.S_IEXEC)

    def build(self):
        self._patch_sources()
        cgo_ldflags = 'CGO_LDFLAGS="-Wl,-z,max-page-size=16384" ' if self.options.page_16k else ""
        self.run(f'{cgo_ldflags}./build.sh android')

    def package(self):
        copy(self, "libxray.aar", src=self.build_folder, dst=os.path.join(self.package_folder, "aar"))

    def package_info(self):
        self.cpp_info.set_property("cmake_extra_variables", {
            "AMNEZIA_LIBXRAY_PATH": os.path.join(self.package_folder, "aar", "libxray.aar"),
        })
