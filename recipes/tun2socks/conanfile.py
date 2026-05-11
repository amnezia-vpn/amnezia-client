from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, chdir
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import VirtualBuildEnv

import os


class Tun2Socks(ConanFile):
    name = "tun2socks"
    version = "2.6.0"
    package_type = "application"
    settings = "os", "arch"
    _binary_name = "tun2socks"

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
            "LDFLAGS=",
            f"GOOS={self._goos}",
            f"GOARCH={goarch}",
        ]

    def _build_go_arch(self, goarch):
        with chdir(self, self.source_folder):
            autotools = Autotools(self)
            autotools.make(self._binary_name, args=self._go_arch_make_args(goarch))

        output_path = os.path.join(self.build_folder, self._binary_name)
        if not os.path.exists(output_path):
            output_path = os.path.join(self.source_folder, self._binary_name)

        arch_output_path = os.path.join(self.build_folder, f"{self._binary_name}-{goarch}")
        os.rename(output_path, arch_output_path)
        return arch_output_path

    def _build_universal_macos(self):
        outputs = [self._build_go_arch(goarch) for goarch in self._goarchs]
        universal_output = os.path.join(self.build_folder, self._binary_name)
        self.run(f"lipo -create {' '.join(outputs)} -output {universal_output}")

    @property
    def _is_windows(self):
        return str(self.settings.get_safe("os")).startswith("Windows")

    @property
    def _ext(self):
        return ".exe" if self._is_windows else ""

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not self._goos or not all(self._goarchs) or self._is_unsupported_multi_arch:
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
        VirtualBuildEnv(self).generate()
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        self._define_go_cache_env(env)
        env.define("LDFLAGS", "")
        env.define("CGO_LDFLAGS", tc.ldflags)
        env.define("CGO_CFLAGS", tc.cflags)
        env.define("GOOS", self._goos)
        if not self._is_universal_macos:
            env.define("GOARCH", self._goarch)
        tc.generate(env)

    def build(self):
        if self._is_universal_macos:
            self._build_universal_macos()
            return

        with chdir(self, self.source_folder):
            at = Autotools(self)
            at.make(self._binary_name)

    def package(self):
        copy(self, self._binary_name, src=self.build_folder, dst=self.package_folder)
        if self._is_windows:
            with chdir(self, self.package_folder):
                os.rename(src=self._binary_name, dst=f"{self._binary_name}.exe")

    def package_info(self):
        self.cpp_info.exe = True
        self.cpp_info.location = os.path.join(self.package_folder, f"tun2socks{self._ext}")
        self.cpp_info.set_property("cmake_target_name", "xjasonlyu::tun2socks")
