from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, chdir
from conan.tools.apple import XCRun
from conan.tools.gnu import Autotools, AutotoolsToolchain

import os
import shlex


class AwgGo(ConanFile):
    name = "awg-go"
    version = "0.2.18"
    package_type = "application"
    settings = "os", "arch"
    _binary_name = "amneziawg-go"

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "Macos": "darwin",
            "Windows": "windows"
        }.get(str(self.settings.os))

    @property
    def _arch_map(self):
        return {
            "x86": "386",
            "x86_64": "amd64",
            "armv8": "arm64"
        }

    @property
    def _archs(self):
        return str(self.settings.arch).split("|")

    @property
    def _goarch(self):
        goarchs = [self._arch_map.get(arch) for arch in self._archs]
        return goarchs[0] if len(goarchs) == 1 else None

    @property
    def _goarchs(self):
        return [self._arch_map.get(arch) for arch in self._archs]

    @property
    def _is_universal_macos(self):
        return str(self.settings.os) == "Macos" and len(self._archs) > 1

    def layout(self):
        basic_layout(self, build_folder=".")

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not self._goos or not all(self._goarchs) or (len(self._archs) > 1 and not self._is_universal_macos):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-go/archive/refs/tags/v{self.version}.zip",
            sha256="58eefbd012e79bd1525f0e02d748979e9480acc1a339df8ceb3b9ffafcedb1ba", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        env.define("GOPATH", os.path.join(self.build_folder, "gopath"))
        env.define("GOMODCACHE", os.path.join(self.build_folder, "gopath", "pkg", "mod"))
        env.define("GOCACHE", os.path.join(self.build_folder, "gocache"))
        env.define("GOTELEMETRY", "off")
        env.define("GOOS", self._goos)
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        tc.generate(env)

    def build(self):
        outputs = []
        with chdir(self, self.source_folder):
            for goarch in self._goarchs:
                arch_destdir = os.path.join(self.build_folder, f"build-{goarch}")
                at = Autotools(self)
                at.make("clean")
                at.make("install", args=[
                    f"DESTDIR={shlex.quote(arch_destdir)}",
                    "BINDIR=",
                    f"GOARCH={goarch}",
                ])
                output_path = os.path.join(arch_destdir, self._binary_name)
                arch_output_path = os.path.join(self.build_folder, f"{self._binary_name}-{goarch}")
                os.rename(output_path, arch_output_path)
                outputs.append(arch_output_path)

        output = os.path.join(self.build_folder, self._binary_name)
        if self._is_universal_macos:
            lipo = XCRun(self).find("lipo")
            self.run("{} -create {} -output {}".format(
                shlex.quote(lipo),
                " ".join(shlex.quote(output) for output in outputs),
                shlex.quote(output)
            ))
            return

        os.rename(outputs[0], output)

    def package(self):
        copy(self, self._binary_name, src=self.build_folder, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, self._binary_name)
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-go")
