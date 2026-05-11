from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir, rename
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.env import VirtualBuildEnv

import os
import shlex


class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.1.0"
    settings = "os", "arch", "compiler"
    _lib_name = "libamnezia_xray.a"

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "iOS": "ios",
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
    def _apple_arch_map(self):
        return {
            "x86_64": "x86_64",
            "armv8": "arm64",
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

    def _go_cache_make_args(self):
        return [f"{name}={value}" for name, value in self._go_cache_vars().items()]

    def _non_arch_flags(self, flags):
        tokens = []
        for flag in flags:
            tokens.extend(shlex.split(flag))

        result = []
        skip_next = False
        for flag in tokens:
            if skip_next:
                skip_next = False
            elif flag == "-arch":
                skip_next = True
            else:
                result.append(flag)
        return result

    def _cgo_flags(self, arch, flags):
        return " ".join(["-arch", self._apple_arch_map[arch]] + self._non_arch_flags(flags))

    def _make_var(self, name, value):
        return f"{name}={shlex.quote(value)}"

    def _go_arch_make_args(self, arch, goarch):
        tc = AutotoolsToolchain(self)
        return self._go_cache_make_args() + [
            f"GOOS={self._goos}",
            f"GOARCH={goarch}",
            f"ARCH={goarch}",
            self._make_var("CGO_CFLAGS", self._cgo_flags(arch, tc.cflags)),
            self._make_var("CGO_LDFLAGS", self._cgo_flags(arch, tc.ldflags)),
        ]

    def _build_go_arch(self, arch, goarch):
        with chdir(self, self.source_folder):
            autotools = Autotools(self)
            autotools.make(args=self._go_arch_make_args(arch, goarch))

        output_path = os.path.join(self.build_folder, self._lib_name)
        if not os.path.exists(output_path):
            output_path = os.path.join(self.source_folder, self._lib_name)

        arch_output_path = os.path.join(self.build_folder, f"libamnezia_xray-{goarch}.a")
        os.rename(output_path, arch_output_path)
        return arch_output_path

    def _build_universal_macos(self):
        outputs = [self._build_go_arch(arch, goarch) for arch, goarch in zip(self._archs, self._goarchs)]
        universal_output = os.path.join(self.build_folder, self._lib_name)
        self.run(f"lipo -create {' '.join(outputs)} -output {universal_output}")

    @property
    def _is_windows(self):
        return str(self.settings.os).startswith("Windows")

    def config_options(self):
        self.package_type = "shared-library" if self._is_windows else "static-library"

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")
        if self._is_windows:
            # mingw-builds is being used on Windows
            del self.settings.compiler

    def layout(self):
        basic_layout(self)

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        if self._is_windows:
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")
            self.tool_requires("mingw-builds/15.1.0")

    def validate(self):
        if not self._goos or not all(self._goarchs) or self._is_unsupported_multi_arch:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/v1.1.0.zip",
            sha256="6ea768ec7002cedd422a39aea17704b888acaf794432aa5937cfc92fb6d80eb5", strip_root=True)

    def generate(self):
        VirtualBuildEnv(self).generate()
        tc = AutotoolsToolchain(self)
        tc.make_args = [
            "LIB_ARC=libamnezia_xray.a"
        ]
        env = tc.environment()
        self._define_go_cache_env(env)
        if not self._is_universal_macos:
            env.define("ARCH", self._goarch)
            env.define("GOARCH", self._goarch)
            env.define("CGO_LDFLAGS", tc.ldflags)
            env.define("CGO_CFLAGS", tc.cflags)
        env.define("GOOS", self._goos)
        if self._is_windows:
            env.define("OS", "windows")
        tc.generate(env)

    def build(self):
        if self._is_universal_macos:
            self._build_universal_macos()
            return

        with chdir(self, self.source_folder):
            autotools = Autotools(self)
            autotools.make()

    def _rename_header(self):
        if not self._is_windows:
            rename(self, os.path.join(self.package_folder, "include", "libamnezia_xray.h"),
                    os.path.join(self.package_folder, "include", "amnezia_xray.h"))

    def _rename_libs(self):
        # workaround of bad naming strategy in amnezia-xray-bindings
        # TODO: change it and kick out the code below
        lib_dir = os.path.join(self.package_folder, "lib")
        for fname in os.listdir(lib_dir):
            if not fname.startswith("lib"):
                src = os.path.join(lib_dir, fname)
                dst = os.path.join(lib_dir, "lib" + fname)
                os.rename(src, dst)

    def package(self):
        copy(self, "*.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        if self._is_universal_macos:
            copy(self, self._lib_name, src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        else:
            copy(self, "*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))
        copy(self, "*.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"))
        self._rename_header()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        self.cpp_info.libs = collect_libs(self)
