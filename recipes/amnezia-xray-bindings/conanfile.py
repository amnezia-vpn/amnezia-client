from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir, rename
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.apple import XCRun, is_apple_os
from conan.tools.apple.apple import _to_apple_arch
from conan.tools.env import Environment

import os
import shlex


class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.3.0"
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
        get(self, f"https://github.com/amnezia-vpn/amnezia-xray-bindings/archive/refs/tags/v{self.version}.zip",
            sha256="97233926c91e0bed61603fddb9909607b97a65f8ff0841a628f96268637ade5c", strip_root=True)

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.apple_arch_flag = None
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
                build_dir = os.path.join(self.build_folder, arch) if self._is_multiarch else self.build_folder
                goarch = self._arch_map.get(arch)

                cflags = list(self._cflags)
                ldflags = list(self._ldflags)
                if is_apple_os(self):
                    cflags.append(f"-arch {_to_apple_arch(arch)}")
                    ldflags.append(f"-arch {_to_apple_arch(arch)}")

                env = Environment()
                env.define("ARCH", goarch)
                env.define("CGO_CFLAGS", " ".join(cflags))
                env.define("CGO_LDFLAGS", " ".join(ldflags))
                with env.vars(self).apply():
                    at = Autotools(self)
                    at.make(args=[
                        f"BUILD_DIR={build_dir.replace("\\", "/") if self._is_windows else build_dir}"
                    ])

            if is_apple_os(self) and self._is_multiarch:
                lipo = XCRun(self).find('lipo')
                archives = [os.path.join(self.build_folder, arch, "amnezia_xray.a") for arch in self._archs]
                output = os.path.join(self.build_folder, "amnezia_xray.a")
                self.run("{} -create -output {} {}".format(
                    shlex.quote(lipo),
                    shlex.quote(output),
                    shlex.join(archives)
                ))

                copy(self, "*.h", os.path.join(self.build_folder, self._archs[0]), self.build_folder)

    def _rename_header(self):
        if not self._is_windows:
            rename(self,
                os.path.join(self.package_folder, "lib", "amnezia_xray.a"),
                os.path.join(self.package_folder, "lib", "libamnezia_xray.a")
            )

    def package(self):
        copy(self, "amnezia_xray.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "amnezia_xray.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "amnezia_xray.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "amnezia_xray.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        self._rename_header()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::xray-bindings")
        self.cpp_info.libs = collect_libs(self)
