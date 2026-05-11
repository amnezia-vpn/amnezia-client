from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import VirtualBuildEnv

import os


class AwgGo(ConanFile):
    name = "awg-go"
    version = "0.2.16"
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

    @property
    def _is_unsupported_multi_arch(self):
        return len(self._archs) > 1 and not self._is_universal_macos

    def _go_cache_vars(self):
        return {
            "GOPATH": os.path.join(self.build_folder, "gopath"),
            "GOMODCACHE": os.path.join(self.build_folder, "gopath", "pkg", "mod"),
            "GOCACHE": os.path.join(self.build_folder, "gocache"),
            "GOTELEMETRY": "off",
        }

    def _define_go_cache_env(self, env):
        for name, value in self._go_cache_vars().items():
            env.define(name, value)

    def _go_arch_make_args(self, goarch):
        return [f"{name}={value}" for name, value in self._go_cache_vars().items()] + [
            f"GOOS={self._goos}",
            f"GOARCH={goarch}",
        ]

    def _build_go_arch(self, goarch):
        autotools = Autotools(self)
        autotools.make(args=self._go_arch_make_args(goarch))

        output_path = os.path.join(self.build_folder, self._binary_name)
        arch_output_path = os.path.join(self.build_folder, f"{self._binary_name}-{goarch}")
        os.rename(output_path, arch_output_path)
        return arch_output_path

    def _build_universal_macos(self):
        outputs = [self._build_go_arch(goarch) for goarch in self._goarchs]
        universal_output = os.path.join(self.build_folder, self._binary_name)
        self.run(f"lipo -create {' '.join(outputs)} -output {universal_output}")

    def layout(self):
        basic_layout(self, build_folder=".")

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not self._goos or not all(self._goarchs) or self._is_unsupported_multi_arch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-go/archive/refs/tags/v{self.version}.zip",
            sha256="34da7d4189f215f3930de441548bc2a0c89d54d347a4fb85cb9c715fce6413aa", strip_root=True
        )

    def generate(self):
        VirtualBuildEnv(self).generate()
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        self._define_go_cache_env(env)
        env.define("GOOS", self._goos)
        if not self._is_universal_macos:
            env.define("GOARCH", self._goarch)
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        tc.generate(env)

    def build(self):
        if self._is_universal_macos:
            self._build_universal_macos()
            return

        at = Autotools(self)
        at.make()

    def package(self):
        copy(self, self._binary_name, src=self.build_folder, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, self._binary_name)
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-go")
