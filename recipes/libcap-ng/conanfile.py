from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, collect_libs
from conan.tools.gnu import Autotools, AutotoolsToolchain, PkgConfigDeps, AutotoolsDeps
from conan.errors import ConanInvalidConfiguration

from pathlib import Path

class LibcapNg(ConanFile):
    name = "libcap-ng"
    version = "0.9.2"
    settings = "build_type", "compiler", "os", "arch"
    options = {
        "shared": [True, False]
    }
    default_options = {
        "shared": False
    }

    def layout(self):
        basic_layout(self, src_folder="src")

    def validate(self):
        if self.settings.os != "Linux":
            ConanInvalidConfiguration(f"{self.name} v{self.version} is available only on Linux")

    def requirements(self):
        self.tool_requires("automake/1.16.5")
        self.tool_requires("libtool/2.4.7")
        self.tool_requires("pkgconf/2.5.1")

    def source(self):
        get(self, f"https://github.com/stevegrubb/libcap-ng/archive/refs/tags/v{self.version}.zip",
            sha256="9c8847f9732f8ee161faf5bebad44cc6f614a8199f3d8dcb2014290b4acedc18", strip_root=True)

    def generate(self):
        pkgconf = PkgConfigDeps(self)
        pkgconf.generate()
        tc = AutotoolsToolchain(self)
        tc.configure_args += ["--without-python3", "--disable-cap-audit"]
        tc.generate()
        deps = AutotoolsDeps(self)
        deps.generate()

    def build(self):
        Path(self.source_folder, "NEWS").touch(exist_ok=True)
        autotools = Autotools(self)
        autotools.autoreconf()
        autotools.configure()
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
