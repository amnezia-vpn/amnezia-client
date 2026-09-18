from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, chdir
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import XCRun
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import Environment
from conan.tools.apple import is_apple_os
from conan.tools.apple.apple import _to_apple_arch

import os
import shlex


class Tun2Socks(ConanFile):
    name = "tun2socks"
    version = "2.6.0"
    package_type = "application"
    settings = "os", "arch"

    _binary_name = "tun2socks"

    _arch_map = {
        "x86": "386",
        "x86_64": "amd64",
        "armv8": "arm64"
    }

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "Macos": "darwin",
            "Windows": "windows"
        }.get(str(self.settings.os))

    @property
    def _archs(self):
        return str(self.settings.arch).split("|")
    
    @property
    def _is_multiarch(self):
        return len(self._archs) > 1

    @property
    def _is_windows(self):
        return str(self.settings.get_safe("os")).startswith("Windows")

    @property
    def _binary_name_ext(self):
        ext = ".exe" if self._is_windows else ""
        return f"{self._binary_name}{ext}"

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not self._goos or not all(arch in self._arch_map for arch in self._archs):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )
        
        if self._is_multiarch and not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support multiarch builds"
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
        tc.apple_arch_flag = None
        env = tc.environment()
        env.define("GOPATH", os.path.join(self.build_folder, "gopath"))
        env.define("GOMODCACHE", os.path.join(self.build_folder, "gopath", "pkg", "mod"))
        env.define("GOCACHE", os.path.join(self.build_folder, "gocache"))
        env.define("GOTELEMETRY", "off")
        env.define("LDFLAGS", "")
        env.define("GOOS", self._goos)
        self._ldflags = tc.ldflags
        self._cflags = tc.cflags
        tc.generate(env)

    def build(self):
        with chdir(self, self.source_folder):
            for arch in self._archs:
                build_dir = os.path.join(self.build_folder, arch) if self._is_multiarch else self.build_folder
                goarch = self._arch_map.get(arch)

                ldflags = self._ldflags
                cflags = self._cflags
                if is_apple_os(self):
                    ldflags.append(f"-arch {_to_apple_arch(arch)}")
                    cflags.append(f"-arch {_to_apple_arch(arch)}")

                env = Environment()
                env.define("GOARCH", goarch)
                env.define("CGO_LDFLAGS", " ".join(ldflags))
                env.define("CGO_CFLAGS", " ".join(cflags))
                with env.vars(self).apply():
                    at = Autotools(self)
                    make_build_dir = build_dir.replace("\\", "/") if self._is_windows else build_dir
                    at.make("tun2socks", args=[
                        f"BUILD_DIR={make_build_dir}"
                    ])
                    if self._is_windows:
                        os.rename(
                            os.path.join(build_dir, self._binary_name),
                            os.path.join(build_dir, self._binary_name_ext)
                        )

        if self._is_multiarch:
            lipo = XCRun(self).find('lipo')
            output = os.path.join(self.build_folder, self._binary_name_ext)
            binaries = [os.path.join(self.build_folder, arch, self._binary_name_ext) for arch in self._archs]
            self.run("{} -create -output {} {}".format(
                shlex.quote(lipo),
                shlex.quote(output),
                shlex.join(binaries)
            ))

    def package(self):
        copy(self, self._binary_name_ext, src=self.build_folder, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, self._binary_name_ext)
        self.cpp_info.set_property("cmake_target_name", "xjasonlyu::tun2socks")
