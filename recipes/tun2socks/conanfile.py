from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, chdir
from conan.errors import ConanInvalidConfiguration
from conan.tools.env import VirtualBuildEnv

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
        # Upstream Makefile: CGO_ENABLED=0 — pure Go; no MSYS2 / MinGW / make on Windows.
        self.tool_requires("go/1.26.0")

    def requirements(self):
        if self._is_windows:
            self.requires("wintun/[*]")

    def source(self):
        get(self, f"https://github.com/xjasonlyu/tun2socks/archive/refs/tags/v{self.version}.zip",
            sha256="a7ef9cec1c30dfe9971af89a8aac767fd3d2a4df833e92b635642c2f0204c701", strip_root=True
        )

    def generate(self):
        VirtualBuildEnv(self).generate()

    def build(self):
        # Makefile default: BUILD_DIR=build, CGO_ENABLED=0, target tun2socks -> build/tun2socks(.exe)
        out_dir = os.path.join(self.source_folder, "build")
        os.makedirs(out_dir, exist_ok=True)
        out_name = f"tun2socks{self._ext}"
        out_path = os.path.join(out_dir, out_name)
        goos = self._goos
        goarch = self._goarch
        with chdir(self, self.source_folder):
            if self._is_windows:
                self.run(
                    f'set "GOOS={goos}"&& set "GOARCH={goarch}"&& set "CGO_ENABLED=0"&& set "GO111MODULE=on"&& '
                    f'go build -trimpath -ldflags="-w -s -buildid=" -o "{out_path}" .',
                    env="conanbuild",
                )
            else:
                self.run(
                    f'GOOS={goos} GOARCH={goarch} CGO_ENABLED=0 GO111MODULE=on '
                    f'go build -trimpath -ldflags="-w -s -buildid=" -o "{out_path}" .',
                    env="conanbuild",
                )

    def package(self):
        out_dir = os.path.join(self.source_folder, "build")
        copy(self, f"tun2socks{self._ext}", src=out_dir, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, f"tun2socks{self._ext}")
        self.cpp_info.set_property("cmake_target_name", "xjasonlyu::tun2socks")
