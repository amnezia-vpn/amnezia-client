from conan import ConanFile
from conan.errors import ConanException, ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy, collect_libs, apply_conandata_patches, export_conandata_patches
from conan.tools.apple import is_apple_os
from conan.tools.gnu import AutotoolsToolchain, Autotools

import os
import struct
from pathlib import Path

class AwgApple(ConanFile):
    name = "awg-apple"
    version = "3.1.4"
    settings = "os", "arch", "compiler"

    @property
    def _goarch(self):
        arch_map = {
            "armv8": "arm64",
            "x86_64": "x86_64",
        }
        archs = str(self.settings.arch).split("|")
        return " ".join(arch_map.get(arch, arch) for arch in archs)

    def export_sources(self):
        export_conandata_patches(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        basic_layout(self, build_folder=os.path.join(self.folders.source, "Sources/WireGuardKitGo"))

    def build_requirements(self):
        self.tool_requires("go/1.26.0")

    def validate(self):
        if not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} does not support {self.settings.os}"
            )

    def source(self):
        get(self, f"https://github.com/amnezia-vpn/amneziawg-apple/archive/refs/tags/v{self.version}.zip",
            sha256="09d7b760d18232fdf121ed2286b2f171b501dc31137e5e7d557c1ee3a99ef772", strip_root=True
        )

    def generate(self):
        tc = AutotoolsToolchain(self)
        sdk = self.settings.get_safe("os.sdk", "macosx")
        tc.make_args = [
            f"ARCHS={self._goarch}",
            f"PLATFORM_NAME={sdk}"
        ]
        tc.generate()

    def build(self):
        apply_conandata_patches(self)
        autotools = Autotools(self)
        autotools.make()
        self._reject_ldapr()
        autotools.make("version-header")

    def _reject_ldapr(self):
        # Fail the iOS prebuild if Clang still emitted LDAPR (FEAT_LRCPC).
        # A10/A11 SIGILL on this encoding inside _cgo_wait_runtime_init_done.
        if str(self.settings.get_safe("os.sdk") or "") != "iphoneos":
            return
        archive = os.path.join(self.build_folder, ".tmp", "wireguard-go-bridge", "libwg-go-arm64.a")
        archive_path = Path(archive)
        if not archive_path.is_file():
            raise ConanException(f"libwg-go-arm64.a not found at {archive}")
        data = archive_path.read_bytes()
        hits = [
            i for i in range(0, len(data) - 3, 4)
            if struct.unpack_from("<I", data, i)[0] & 0xFFFFFC00 == 0xF8BFC000
        ]
        if hits:
            raise ConanException(f"LDAPR still present at {len(hits)} site(s)")

    def package(self):
        copy(self, "wireguard.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.h", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.a", src=os.path.join(self.build_folder, "out"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::awg-apple")
        self.cpp_info.libs = collect_libs(self)
