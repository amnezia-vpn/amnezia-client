from conan import ConanFile
from conan.tools.files import get, copy, collect_libs, chdir, rename
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.apple import XCRun, is_apple_os
from conan.tools.apple.apple import _to_apple_arch
from conan.tools.env import Environment, VirtualBuildEnv

import os
import shlex
import sys

_recipe_dir = os.path.dirname(os.path.abspath(__file__))
if _recipe_dir not in sys.path:
    sys.path.insert(0, _recipe_dir)

from windows_cgo import ensure_msvc_import_lib, is_windows_arm64, run_llvm_mingw_go_build


class AmneziaXrayBindings(ConanFile):
    name = "amnezia-xray-bindings"
    version = "1.4.0"
    settings = "os", "arch", "compiler"
    exports = "windows_cgo.py"

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

    @property
    def _windows_arm64(self):
        return is_windows_arm64(self)

    def config_options(self):
        self.package_type = "shared-library" if self._is_windows else "static-library"

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")
        if self._is_windows and not self._windows_arm64:
            # mingw-builds is being used on Windows
            del self.settings.compiler
        # Windows ARM64 keeps the compiler setting so vcvars can expose cl/lib to CGO

    def layout(self):
        basic_layout(self)

    def build_requirements(self):
        self.tool_requires("go/1.26.0")
        # Windows ARM64 builds with llvm-mingw + MSVC lib.exe (see windows_cgo.py);
        # msys2 / mingw-builds have no arm64 packages.
        if self._is_windows and not self._windows_arm64:
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
            sha256="8977896bba99f1a3bad61d734b2929ec3d01c3ca0e206ee8ce5eb013d38ab118", strip_root=True)

    def generate(self):
        if self._windows_arm64:
            # Go on PATH; the CGO toolchain is set up in build().
            VirtualBuildEnv(self).generate()
            return

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
        if self._windows_arm64:
            self._build_windows_arm64()
            return

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

    def _build_windows_arm64(self):
        """c-shared DLL via llvm-mingw CGO + an MSVC import library for the app."""
        dll_name = "amnezia_xray.dll"
        dll_path = os.path.join(self.build_folder, dll_name)
        run_llvm_mingw_go_build(
            self, self.source_folder, dll_path, self._arch_map[self._archs[0]]
        )
        ensure_msvc_import_lib(self, self.build_folder, dll_name, "amnezia_xray")

        def_path = os.path.join(self.build_folder, "amnezia_xray.def")
        if os.path.isfile(def_path):
            os.remove(def_path)

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
