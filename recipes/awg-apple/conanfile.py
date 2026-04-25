from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, collect_libs, replace_in_file
from conan.tools.apple import is_apple_os
from conan.tools.gnu import AutotoolsToolchain, Autotools

import os
import subprocess

TVOS_PREBUILT_DIR_ENV = "AMNEZIA_TVOS_AWG_PREBUILT_DIR"
TVOS_VERSION_HEADER_DIR_ENV = "AMNEZIA_TVOS_AWG_VERSION_HEADER_DIR"


class AwgApple(ConanFile):
    name = "awg-apple"
    version = "2.0.1"
    settings = "os", "arch"
    options = {
        "as_framework": [True, False]
    }
    default_options = {
        "as_framework": False
    }

    @property
    def _is_tvos(self):
        return str(self.settings.os) == "tvOS"

    @property
    def _goarch(self):
        arch_map = {
            "armv8": "arm64",
            "x86_64": "x86_64",
        }
        archs = str(self.settings.arch).split("|")
        return " ".join(arch_map.get(arch, arch) for arch in archs)

    def layout(self):
        basic_layout(self, build_folder=os.path.join(self.folders.source, "Sources/WireGuardKitGo"))

    def validate(self):
        if not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-apple/archive/refs/tags/v{self.version}.zip",
            sha256="9fe4f8cfbb6a751558b54b7979db3a5ea46e49731912aae99f093e84a1433e97", strip_root=True
        )

    def _go_env(self):
        env = os.environ.copy()
        env["GOTOOLCHAIN"] = "local"
        return env

    def _go_root(self):
        try:
            return subprocess.check_output(
                ["go", "env", "GOROOT"],
                text=True,
                env=self._go_env(),
                cwd="/",
            ).strip()
        except (FileNotFoundError, subprocess.CalledProcessError) as error:
            raise ConanInvalidConfiguration(
                "Go is required to build awg-apple. Install Go or provide a tvOS "
                f"prebuilt bridge through {TVOS_PREBUILT_DIR_ENV}."
            ) from error

    def _env_path(self, name):
        value = os.environ.get(name)
        if not value:
            return None
        return os.path.abspath(os.path.expanduser(value))

    def _uses_tvos_prebuilt_bridge(self):
        return self._is_tvos and self._env_path(TVOS_PREBUILT_DIR_ENV) is not None

    def _make_args(self):
        if self._uses_tvos_prebuilt_bridge():
            return []

        sdk = self.settings.get_safe("os.sdk", "macosx")
        args = [
            f"ARCHS={self._goarch}",
            f"PLATFORM_NAME={sdk}",
            f"REAL_GOROOT={self._go_root()}",
            "GOTOOLCHAIN=local",
        ]
        if self._is_tvos:
            args.extend([
                "GOOS_appletvos=ios",
                "GOFLAGS=-tags=netgo",
                "DEPLOYMENT_TARGET_CLANG_FLAG_NAME=mtvos-version-min",
                "DEPLOYMENT_TARGET_CLANG_ENV_NAME=TVOS_DEPLOYMENT_TARGET",
                f"TVOS_DEPLOYMENT_TARGET={self.settings.os.version}",
            ])
        return args

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.make_args = self._make_args()
        tc.generate()

    def _copy_tvos_prebuilt_bridge(self):
        prebuilt_dir = self._env_path(TVOS_PREBUILT_DIR_ENV)
        if not prebuilt_dir:
            return False

        lib_path = os.path.join(prebuilt_dir, "libwg-go.a")
        if not os.path.isfile(lib_path):
            raise ConanInvalidConfiguration(
                f"{TVOS_PREBUILT_DIR_ENV} is set to '{prebuilt_dir}', but libwg-go.a was not found there."
            )

        header_dir = self._env_path(TVOS_VERSION_HEADER_DIR_ENV) or prebuilt_dir
        header_path = os.path.join(header_dir, "wireguard-go-version.h")
        if not os.path.isfile(header_path):
            raise ConanInvalidConfiguration(
                f"wireguard-go-version.h was not found in '{header_dir}'. Set "
                f"{TVOS_VERSION_HEADER_DIR_ENV} if the header lives outside {TVOS_PREBUILT_DIR_ENV}."
            )

        self.output.warning(
            f"Using explicit tvOS WireGuard bridge from {TVOS_PREBUILT_DIR_ENV}={prebuilt_dir}"
        )
        out_dir = os.path.join(self.build_folder, "out")
        copy(self, "libwg-go.a", src=prebuilt_dir, dst=out_dir)
        copy(self, "wireguard-go-version.h", src=header_dir, dst=out_dir)
        return True

    def _patch_tvos_go_version_for_local_toolchain(self):
        replace_in_file(self, os.path.join(self.build_folder, "go.mod"), "go 1.26", "go 1.25")

    def build(self):
        if self._is_tvos:
            if self._copy_tvos_prebuilt_bridge():
                return
            self._patch_tvos_go_version_for_local_toolchain()

        autotools = Autotools(self)
        autotools.make()
        autotools.make("version-header")

    def package(self):
        copy(self, "wireguard.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.h", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-apple")
        self.cpp_info.libs = collect_libs(self)
