from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, chdir
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import Environment

import os

class Tun2Socks(ConanFile):
    name = "tun2socks"
    version = "2.6.0"
    package_type = "application"
    settings = "os", "arch"

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "Macos": "darwin",
            "Windows": "windows"
        }.get(str(self.settings.os))

    @property
    def _goarch(self):
        return {
            "x86": "386",
            "x86_64": "amd64",
            "armv8": "arm64"
        }.get(str(self.settings.arch))
    
    @property
    def _is_windows(self):
        return str(self.settings.get_safe("os")).startswith("Windows")
    
    @property
    def _ext(self):
        return ".exe" if self._is_windows else ""

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not self._goos or not self._goarch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self._is_windows:
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")
            self.tool_requires("mingw-builds/15.1.0")

    def requirements(self):
        if self._is_windows:
            self.requires("wintun/[*]")

    def source(self):
        get(self, f"https://github.com/xjasonlyu/tun2socks/archive/refs/tags/v{self.version}.zip",
            sha256="a7ef9cec1c30dfe9971af89a8aac767fd3d2a4df833e92b635642c2f0204c701", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        env.define("LDFLAGS", "")
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        env.define("GOOS", self._goos)
        env.define("GOARCH", self._goarch)
        tc.generate(env)

    def build(self):
        with chdir(self, self.source_folder):
            at = Autotools(self)
            at.make("tun2socks")

    def package(self):
        copy(self, "tun2socks", src=self.build_folder, dst=self.package_folder)
        if self._is_windows:
            with chdir(self, self.package_folder):
                os.rename(src="tun2socks", dst="tun2socks.exe")

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, f"tun2socks{self._ext}")
        self.cpp_info.set_property("cmake_target_name", "xjasonlyu::tun2socks")
