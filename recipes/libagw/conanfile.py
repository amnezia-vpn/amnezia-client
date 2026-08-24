from conan import ConanFile
from conan.tools.files import copy, chdir
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.gnu import AutotoolsToolchain
from conan.tools.apple import XCRun, is_apple_os
from conan.tools.apple.apple import _to_apple_arch
from conan.tools.env import Environment
from conan.tools.scm import Git

import os
import shlex


class Libagw(ConanFile):
    name = "libagw"
    version = "1.0.0"
    settings = "os", "arch", "compiler"

    _arch_map = {
        "x86": "386",
        "x86_64": "amd64",
        "armv7": "arm",
        "armv8": "arm64"
    }

    @property
    def _goos(self):
        return {
            "Linux": "linux",
            "Android": "android",
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
        # MSVC cannot link a Go c-archive (LNK1223: the gcc-generated .pdata
        # contributions are rejected), so Windows gets a DLL plus an import lib.
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
        # 1.23 line: go>=1.24 has an android/arm futex_time64 regression
        # (golang/go#77930), the same reason openvpn-pt-android pins it.
        self.tool_requires("go/1.23.12")
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
        git = Git(self)
        git.clone(url="https://github.com/amnezia-vpn/libagw.git", target=".",
                  args=["--branch", f"v{self.version}"])

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.apple_arch_flag = None
        env = tc.environment()
        env.define("GOPATH", os.path.join(self.build_folder, "gopath"))
        env.define("GOMODCACHE", os.path.join(self.build_folder, "gopath", "pkg", "mod"))
        env.define("GOCACHE", os.path.join(self.build_folder, "gocache"))
        env.define("GOOS", self._goos)
        env.define("CGO_ENABLED", "1")
        self._ldflags = tc.ldflags
        self._cflags = tc.cflags
        tc.generate(env)

    def _go_build(self, output, buildmode, goarch, cflags, ldflags):
        env = Environment()
        env.define("GOARCH", goarch)
        env.define("CGO_CFLAGS", " ".join(cflags))
        env.define("CGO_LDFLAGS", " ".join(ldflags))
        with env.vars(self).apply():
            self.run(f'go build -buildmode={buildmode} -ldflags="-s -w" -o {shlex.quote(output)} ./archive')

    def build(self):
        with chdir(self, self.source_folder):
            for arch in self._archs:
                build_dir = os.path.join(self.build_folder, "slices", arch) if self._is_multiarch else self.build_folder
                if self._is_windows:
                    build_dir = build_dir.replace("\\", "/")
                goarch = self._arch_map.get(arch)

                cflags = list(self._cflags)
                ldflags = list(self._ldflags)
                if is_apple_os(self):
                    cflags.append(f"-arch {_to_apple_arch(arch)}")
                    ldflags.append(f"-arch {_to_apple_arch(arch)}")

                if self._is_windows:
                    dll = f"{build_dir}/agw.dll"
                    self._go_build(dll, "c-shared", goarch, cflags, ldflags)
                    # An import lib MSVC accepts: gendef reads the DLL exports,
                    # dlltool turns them into agw.lib.
                    with chdir(self, build_dir):
                        self.run("gendef agw.dll")
                        self.run("dlltool -d agw.def -l agw.lib -D agw.dll")
                else:
                    self._go_build(os.path.join(build_dir, "libagw.a"), "c-archive", goarch, cflags, ldflags)

            if is_apple_os(self) and self._is_multiarch:
                lipo = XCRun(self).find("lipo")
                archives = [os.path.join(self.build_folder, "slices", arch, "libagw.a") for arch in self._archs]
                output = os.path.join(self.build_folder, "libagw.a")
                self.run("{} -create -output {} {}".format(
                    shlex.quote(lipo),
                    shlex.quote(output),
                    shlex.join(archives)
                ))

    def package(self):
        headers = os.path.join(self.source_folder, "cabi")
        copy(self, "agw.h", src=headers, dst=os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "agw_types.h", src=headers, dst=os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "libagw.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"),
             keep_path=False, excludes=["slices/*"])
        copy(self, "agw.lib", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "agw.dll", src=self.build_folder, dst=os.path.join(self.package_folder, "bin"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::libagw")
        self.cpp_info.libs = ["agw"]
        if is_apple_os(self):
            # Go's crypto/x509 reads the system trust store.
            self.cpp_info.frameworks = ["CoreFoundation", "Security"]
        elif str(self.settings.os) == "Android":
            self.cpp_info.system_libs = ["log"]
        elif str(self.settings.os) == "Linux":
            self.cpp_info.system_libs = ["pthread"]
