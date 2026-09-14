from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, chdir
from conan.tools.apple import XCRun
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.apple import is_apple_os
from conan.tools.apple.apple import _to_apple_arch
from conan.tools.env import Environment

import os
import shlex


class AwgGo(ConanFile):
    name = "awg-go"
    version = "3.1.20260814"
    package_type = "application"
    settings = "os", "arch"

    _binary_name = "amneziawg-go"

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

    def layout(self):
        basic_layout(self, build_folder=".")

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not self._goos or not all(arch in self._arch_map for arch in self._archs):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )
        
        if self._is_multiarch and not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support multiarch builds"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-go/archive/refs/tags/v{self.version}.zip",
            sha256="a95853baa25d438a3e92ea5207bd315e3a45143b5209488ebf7f0b44e2e2bcc3", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.apple_arch_flag = None
        env = tc.environment()
        env.define("GOPATH", os.path.join(self.build_folder, "gopath"))
        env.define("GOMODCACHE", os.path.join(self.build_folder, "gopath", "pkg", "mod"))
        env.define("GOCACHE", os.path.join(self.build_folder, "gocache"))
        env.define("GOTELEMETRY", "off")
        env.define("GOOS", self._goos)
        self._ldflags = tc.ldflags
        self._cflags = tc.cflags
        tc.generate(env)

    def build(self):
        with chdir(self, self.source_folder):
            for arch in self._archs:
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
                    at.make()
                    if self._is_multiarch:
                        os.rename(
                            os.path.join(self.source_folder, self._binary_name),
                            os.path.join(self.source_folder, f"{self._binary_name}-{arch}")
                        )

            if is_apple_os(self) and self._is_multiarch:
                lipo = XCRun(self).find("lipo")
                output = os.path.join(self.build_folder, self._binary_name)
                binaries = [os.path.join(self.build_folder, f"{self._binary_name}-{arch}") for arch in self._archs]
                self.run("{} -create -output {} {}".format(
                    shlex.quote(lipo),
                    shlex.quote(output),
                    shlex.join(binaries)
                ))

    def package(self):
        with chdir(self, self.source_folder):
            at = Autotools(self)
            at.make("install", args=[
                f"BINDIR={self.package_folder}",
            ])

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, self._binary_name)
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-go")
