from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, load, replace_in_file, rmdir, save
from conan.tools.env import VirtualBuildEnv
from conan.errors import ConanException, ConanInvalidConfiguration
from conan.tools.scm import Git

import os
import platform
import re
import shutil
from io import StringIO
from pathlib import Path

class AwgAndroid(ConanFile):
    name = "awg-android"
    version = "3.1.20260814"
    settings = "os", "arch", "build_type", "compiler"

    _WINDOWS_GO_HOST_PLATFORM = "windows-amd64"

    # Go used to build libwg-go on every platform (downloaded by the Makefile
    # on Linux/macOS, provided by conan on Windows); must satisfy the `go`
    # directives of tunnel/tools/libwg-go/go.mod and its dependencies,
    # otherwise `go build` (with GOTOOLCHAIN=local) refuses to build.
    _GO_VERSION = "1.25.0"
    _GO_TARBALL_SHA256 = {
        "darwin-amd64": "5bd60e823037062c2307c71e8111809865116714d6f6b410597cf5075dfd80ef",
        "darwin-arm64": "544932844156d8172f7a28f77f2ac9c15a23046698b6243f633b0a0b00c0749c",
        "linux-amd64": "2852af0cb20a13139b3448992e69b868e50ed0f8a1e5940ee1de9e19a123b613",
    }

    def export_sources(self):
        export_conandata_patches(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.4.1 <4]")
        if platform.system() == "Windows":
            self.tool_requires("ninja/[*]")
            self.tool_requires(f"go/{self._GO_VERSION}")
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} v{self.version} does not support {self.settings.os}")

    def source(self):
        for entry in os.listdir(self.source_folder):
            if entry == "patches":
                continue
            path = os.path.join(self.source_folder, entry)
            shutil.rmtree(path) if os.path.isdir(path) else os.remove(path)
        git = Git(self)
        git.clone(
            url="https://github.com/amnezia-vpn/amneziawg-android.git",
            target="checkout",
            args=["--recurse-submodules", "--branch", f"v{self.version}"]
        )
        checkout_dir = os.path.join(self.source_folder, "checkout")
        for entry in os.listdir(checkout_dir):
            shutil.move(os.path.join(checkout_dir, entry), os.path.join(self.source_folder, entry))
        os.rmdir(checkout_dir)

    def generate(self):
        venv = VirtualBuildEnv(self)
        # Never let `go` auto-download another toolchain: it would silently
        # replace the runtime-patched Go prepared for this build, and with
        # GOSUMDB=off the download cannot be verified and fails anyway.
        venv.environment().define("GOTOOLCHAIN", "local")
        # sum.golang.org is sometimes unreachable from CI runners entirely
        # ("connection refused"); go.sum already pins the exact hashes we
        # need, so the extra public-transparency-log check isn't required.
        venv.environment().define("GOSUMDB", "off")
        venv.generate()

        tc = CMakeToolchain(self)
        tc.variables["GRADLE_USER_HOME"] = Path(os.path.join(self.build_folder, "gradle_user_home")).as_posix()
        tc.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = Path(os.path.join(self.build_folder, "out")).as_posix()
        # not to warn in case of strtok() usage
        tc.extra_cflags = ["-Wno-deprecated-declarations"]
        tc.generate()

    def _patch_sources(self):
        if platform.system() == 'Windows':
            for rel_path in (
                os.path.join("tunnel", "tools", "CMakeLists.txt"),
                os.path.join("tunnel", "tools", "libwg-go", "Makefile"),
            ):
                abs_path = os.path.join(self.source_folder, rel_path)
                content = load(self, abs_path)
                if "\r\n" in content:
                    save(self, abs_path, content.replace("\r\n", "\n"))

        self._sync_makefile_go_version()

        if platform.system() == 'Darwin':
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'flock "$@.lock" -c \' \\\n',
                "",
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'mv "$@.tmp" "$@"\'',
                'mv "$@.tmp" "$@"',
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'touch "$@"\'',
                'touch "$@"',
            )
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                'sha256sum -c',
                'shasum -a 256 -c'
            )
        elif platform.system() == 'Windows':
            apply_conandata_patches(self)
            self._prepare_patched_go_for_windows()

    def _sync_makefile_go_version(self):
        makefile_path = os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile")
        content = load(self, makefile_path)
        content, n = re.subn(r"^GO_VERSION := \S+", f"GO_VERSION := {self._GO_VERSION}",
                             content, count=1, flags=re.MULTILINE)
        if n != 1:
            raise ConanException("Could not find GO_VERSION in libwg-go/Makefile; "
                                  "upstream may have restructured it, recipe needs updating")
        for go_platform, sha256 in self._GO_TARBALL_SHA256.items():
            content, n = re.subn(rf"^GO_HASH_{go_platform} := \S+", f"GO_HASH_{go_platform} := {sha256}",
                                 content, count=1, flags=re.MULTILINE)
            if n != 1:
                raise ConanException(f"Could not find GO_HASH_{go_platform} in libwg-go/Makefile; "
                                      "upstream may have restructured it, recipe needs updating")
        save(self, makefile_path, content)

    def _prepare_patched_go_for_windows(self):
        # _sync_makefile_go_version has pinned GO_VERSION in the Makefile to
        # self._GO_VERSION, so the paths below match what make expects
        go_version = self._GO_VERSION
        build_dir = os.path.join(self.build_folder, "generated-src")
        go_dir = os.path.join(build_dir, f"go-{go_version}")
        prepared_marker = os.path.join(go_dir, ".prepared")

        if os.path.exists(prepared_marker):
            return
        gradle_user_home = os.path.join(self.build_folder, "gradle_user_home")
        tarball_path = os.path.join(gradle_user_home, "caches", "golang",
                                     f"go{go_version}.{self._WINDOWS_GO_HOST_PLATFORM}.tar.gz")
        os.makedirs(os.path.dirname(tarball_path), exist_ok=True)
        save(self, tarball_path, "")

        goroot_output = StringIO()
        self.run("go env GOROOT", env="conanbuild", stdout=goroot_output)
        goroot = goroot_output.getvalue().strip()

        os.makedirs(build_dir, exist_ok=True)
        if os.path.isdir(go_dir):
            rmdir(self, go_dir)
        shutil.copytree(goroot, go_dir)

        # the same command the Makefile runs on Linux/macOS after unpacking Go
        diff_path = os.path.join(self.source_folder, "tunnel", "tools", "libwg-go",
                                  "goruntime-boottime-over-monotonic.diff")
        self.run('patch -p1 -f -N -r- -d "{}" -i "{}"'.format(
            Path(go_dir).as_posix(), Path(diff_path).as_posix()), env="conanbuild")

        save(self, prepared_marker, "")

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure(build_script_folder=os.path.join(self.source_folder, "tunnel", "tools"))
        cmake.build(target=["libwg-go.so", "libwg.so", "libwg-quick.so"])

    def package(self):
        copy(self, "libwg-go.h", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "libwg-go.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))
        copy(self, "libwg.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "bin"))
        copy(self, "libwg-quick.so", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-android")
        self.cpp_info.libs = [ "wg-go" ]
        self.cpp_info.set_property("cmake_extra_variables", {
            "AMNEZIA_ANDROID_LIBWG_PATH": Path(os.path.join(self.package_folder, "bin", "libwg.so")).as_posix(),
            "AMNEZIA_ANDROID_LIBWG_QUICK_PATH": Path(os.path.join(self.package_folder, "bin", "libwg-quick.so")).as_posix(),
        })
