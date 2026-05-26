from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir, rename, mkdir
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.apple import XCRun, is_apple_os
from conan.tools.apple.apple import _to_apple_arch

import os
import shlex
from pathlib import Path


class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.1.0"
    settings = "os", "arch", "compiler"

    _arch_map = {
        "x86": "386",
        "x86_64": "amd64",
        "armv8": "arm64"
    }

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "iOS": "ios",
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
        if not self._goos or not all(arch in self._arch_map for arch in self._archs):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os} {self.settings.arch}"
            )

        if self._is_multiarch and not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support multiarch builds"
            )

    def source(self):
        get(self, "https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/v1.1.0.zip",
            sha256="6ea768ec7002cedd422a39aea17704b888acaf794432aa5937cfc92fb6d80eb5", strip_root=True)

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.apple_arch_flag = None
        tc.make_args = [
            "LIB_ARC=libamnezia_xray.a"
        ]
        env = tc.environment()
        env.define("GOPATH", os.path.join(self.build_folder, "gopath"))
        env.define("GOMODCACHE", os.path.join(self.build_folder, "gopath", "pkg", "mod"))
        env.define("GOCACHE", os.path.join(self.build_folder, "gocache"))
        env.define("GOOS", self._goos)
        if self._is_windows:
            env.define("OS", "windows")
        self._ldflags = tc.ldflags
        self._cflags = tc.cflags
        tc.generate(env)

    def build(self):
        with chdir(self, self.source_folder):
            for arch in self._archs:
                cflags = list(self._cflags)
                ldflags = list(self._ldflags)
                if is_apple_os(self):
                    cflags.append(f"-arch {_to_apple_arch(arch)}")
                    ldflags.append(f"-arch {_to_apple_arch(arch)}")

                autotools = Autotools(self)
                autotools.make(args=[
                    f"BUILD_DIR={os.path.join('build', arch)}",
                    f"ARCH={self._arch_map.get(arch)}",
                    f"CGO_CFLAGS={shlex.quote(' '.join(cflags))}",
                    f"CGO_LDFLAGS={shlex.quote(' '.join(ldflags))}"
                ])

            if is_apple_os(self) and self._is_multiarch:
                merged_dir = os.path.join(self.source_folder, "merged")
                archives = sorted(
                    str(path) for path in Path(self.source_folder).glob("build/*/*.a")
                )
                if not archives:
                    raise ConanInvalidConfiguration(
                        f"{self.name} v{self.version} did not produce archives for {self.settings.arch}"
                    )
                output = os.path.join(merged_dir, Path(archives[0]).name)

                mkdir(self, merged_dir)
                lipo = XCRun(self).find('lipo')
                self.run("{} -create -output {} {}".format(
                    shlex.quote(lipo),
                    shlex.quote(output),
                    " ".join(shlex.quote(archive) for archive in archives)
                ))

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
        header_src = os.path.join(self.source_folder, "build", self._archs[0])
        lib_src = os.path.join(self.source_folder, "merged") if self._is_multiarch else header_src
        copy(self, "*.h", src=header_src, dst=os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "*.a", src=lib_src, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.lib", src=lib_src, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dll", src=lib_src, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        self._rename_header()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        self.cpp_info.libs = collect_libs(self)
