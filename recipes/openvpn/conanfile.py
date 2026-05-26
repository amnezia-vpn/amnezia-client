from conan import ConanFile
from conan.tools.files import get, copy, replace_in_file
from conan.tools.gnu import Autotools, AutotoolsToolchain, AutotoolsDeps, PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMake, CMakeDeps

import os

class Openvpn(ConanFile):
    name = "openvpn"
    version = "2.7.0"
    package_type = "application"
    settings = "os", "build_type", "arch", "compiler"

    @property
    def _is_windows(self):
        return str(self.settings.os).startswith("Windows")

    def export_sources(self):
        copy(self, "*applink.c", src=self.recipe_folder, dst=self.export_sources_folder)

    def layout(self):
        if self._is_windows:
            cmake_layout(self)
        else:
            basic_layout(self)

    def build_requirements(self):
        if self._is_windows:
            self.tool_requires("cmake/[>=3.14 <4]")
        else:
            self.tool_requires("libtool/2.4.7")
            self.tool_requires("automake/1.16.5")

        if self.settings.os == "Linux" or self._is_windows:
            self.tool_requires("pkgconf/2.5.1")

    def requirements(self):
        openssl_opts = {"no_asm": True} if self._is_windows and str(self.settings.arch) == "armv8" else {}
        self.requires("openssl/3.6.1", visible=False, options=openssl_opts)
        self.requires("lz4/1.10.0", visible=False)
        self.requires("lzo/2.10", visible=False)
        if self.settings.os == "Linux":
            self.requires("libnl/3.9.0", visible=False)
            self.requires("libcap-ng/0.9.2", visible=False)
        if self._is_windows:
            self.requires("tap-windows6/[*]")

    def source(self):
        get(self, f"https://github.com/OpenVPN/openvpn/archive/refs/tags/v{self.version}.zip",
            sha256="1a65d8587f932c13d55b1f175ff2e1d61d795d9092788662e888054854d4ee3d", strip_root=True
        )

    def _patch_sources(self):
        cmakelists = os.path.join(self.source_folder, "CMakeLists.txt")
        replace_in_file(self, cmakelists, "/Qspectre", "")
        # Empty CMAKE_GENERATOR_PLATFORM (Ninja + MSVC) breaks
        # `if (${CMAKE_GENERATOR_PLATFORM} STREQUAL ...)` → invalid `if (STREQUAL ...)`.
        replace_in_file(
            self,
            cmakelists,
            'if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64" OR ${CMAKE_GENERATOR_PLATFORM} STREQUAL "x86")',
            'if (CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "x86" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")',
        )

    def generate(self):
        self._patch_sources()

        if self.settings.os == "Linux" or self._is_windows:
            pkgconf = PkgConfigDeps(self)
            pkgconf.generate()

        if self._is_windows:
            tc = CMakeToolchain(self)
            applink_include_path = os.path.join(self.export_sources_folder, "include").replace("\\", "/")
            tap_include_path = (self.dependencies["tap-windows6"].cpp_info.aggregated_components().includedirs[0]).replace("\\", "/")
            tc.extra_cflags = [ f"-I{tap_include_path}", f"-I{applink_include_path}" ]
            tc.extra_cxxflags = [ f"-I{tap_include_path}", f"-I{applink_include_path}" ]
            tc.cache_variables["BUILD_TESTING"] = False
            tc.cache_variables["ENABLE_PKCS11"] = False
            # Upstream defaults USE_WERROR=ON → /WX on MSVC; breaks on newer toolchains (warnings from deps / OpenVPN).
            tc.cache_variables["USE_WERROR"] = False
            tc.generate()
            deps = CMakeDeps(self)
            deps.generate()
        else:
            tc = AutotoolsToolchain(self)
            tc.configure_args.extend(["--disable-shared", "--enable-static"])
            tc.configure_args.append("--disable-plugins")
            tc.generate()
            deps = AutotoolsDeps(self)
            deps.generate()

    def build(self):
        if self._is_windows:
            cmake = CMake(self)
            cmake.configure()
            cmake.build()
        else:
            at = Autotools(self)
            at.autoreconf()
            at.configure()
            at.make()

    def package(self):
        if self._is_windows:
            copy(self, "*openvpn.exe", src=self.build_folder, dst=self.package_folder, keep_path=False)
        else:
            copy(self, "openvpn", src=os.path.join(self.build_folder, "src", "openvpn"), dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True

        ext = ".exe" if self._is_windows else ""
        self.cpp_info.location = os.path.join(self.package_folder, f"openvpn{ext}")
