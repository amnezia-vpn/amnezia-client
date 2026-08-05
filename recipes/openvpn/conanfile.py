from conan import ConanFile
from conan.tools.files import get, copy, replace_in_file, apply_conandata_patches, export_conandata_patches
from conan.tools.gnu import Autotools, AutotoolsToolchain, AutotoolsDeps, PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMake, CMakeDeps

import glob
import os

class Openvpn(ConanFile):
    name = "openvpn"
    version = "2.7.5"
    package_type = "application"
    settings = "os", "build_type", "arch", "compiler"

    @property
    def _is_windows(self):
        return str(self.settings.os).startswith("Windows")

    def export_sources(self):
        export_conandata_patches(self)
        copy(self, "*applink.c", src=self.recipe_folder, dst=self.export_sources_folder)

    def layout(self):
        if self._is_windows:
            cmake_layout(self)
        else:
            basic_layout(self)

    def build_requirements(self):
        if self._is_windows:
            self.tool_requires("cmake/[>=4.2]")
        else:
            self.tool_requires("libtool/2.4.7")
            self.tool_requires("automake/1.16.5")

        if self.settings.os == "Linux" or self._is_windows:
            self.tool_requires("pkgconf/2.5.1")

    def requirements(self):
        self.requires("openssl/3.6.2", visible=False)
        self.requires("lz4/1.10.0", visible=False)
        self.requires("lzo/2.10", visible=False)
        if self.settings.os == "Linux":
            self.requires("libnl/3.9.0", visible=False)
            self.requires("libcap-ng/0.9.2", visible=False)
        if self._is_windows:
            self.requires("tap-windows6/[*]")

    def source(self):
        get(self, f"https://github.com/OpenVPN/openvpn/archive/refs/tags/v{self.version}.zip",
            sha256="8b005fb1b4fd008c0e0b8dbd618498efcda89e5843b59364d970ede254f3a049", strip_root=True
        )

    def _patch_sources(self):
        replace_in_file(self,
            os.path.join(self.source_folder, "CMakeLists.txt"),
            "/Qspectre",
            ""
        )

    def _find_mc_compiler(self):
        # src/openvpnserv needs the Windows message compiler; with the VS
        # generator nothing puts the SDK bin dir on PATH unless the console
        # ran vcvars, so resolve mc.exe explicitly and let find_program use it
        host = {"ARM64": "arm64", "AMD64": "x64", "x86": "x86"}.get(
            os.environ.get("PROCESSOR_ARCHITECTURE", ""), "x64")
        candidates = []
        sdk_bin = os.environ.get("WindowsSdkVerBinPath")
        if sdk_bin:
            candidates.append(os.path.join(sdk_bin, host, "mc.exe"))
        pf86 = os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)")
        candidates.extend(sorted(
            glob.glob(os.path.join(pf86, "Windows Kits", "10", "bin", "10.*", host, "mc.exe")),
            reverse=True))
        for candidate in candidates:
            if os.path.exists(candidate):
                return candidate.replace("\\", "/")
        return None

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
            mc_compiler = self._find_mc_compiler()
            if mc_compiler:
                tc.cache_variables["MC_COMPILER"] = mc_compiler
            tc.generate()
            deps = CMakeDeps(self)
            deps.generate()
        else:
            tc = AutotoolsToolchain(self)
            tc.configure_args.extend(["--disable-shared", "--enable-static"])
            tc.configure_args.append("--disable-plugins")
            if self.settings.os == "Linux":
                openssl_libdir = self.dependencies["openssl"].cpp_info.aggregated_components().libdirs[0]
                # pad the rpath so consumers can rewrite it in place (it cannot grow)
                padding = max(0, 256 - len(openssl_libdir) - 2)
                rpath = f"{openssl_libdir}:/" + "_" * padding
                tc.extra_ldflags.append(f"-Wl,-rpath,{rpath}")
            elif self.settings.os == "Macos":
                # reserve header space so consumers can rewrite rpaths via install_name_tool
                tc.extra_ldflags.append("-Wl,-headerpad_max_install_names")
            tc.generate()
            deps = AutotoolsDeps(self)
            deps.generate()

    def build(self):
        apply_conandata_patches(self)
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
            # tapctl creates/deletes adapters for both tap-windows6 and
            # ovpn-dco hwids; the service uses it to provision the DCO adapter
            copy(self, "*tapctl.exe", src=self.build_folder, dst=self.package_folder, keep_path=False)
        else:
            copy(self, "openvpn", src=os.path.join(self.build_folder, "src", "openvpn"), dst=self.package_folder)

    def package_info(self):
        self.cpp_info.exe = True

        ext = ".exe" if self._is_windows else ""
        self.cpp_info.location = os.path.join(self.package_folder, f"openvpn{ext}")
        if self._is_windows:
            self.cpp_info.set_property("cmake_extra_variables", {
                "OPENVPN_TAPCTL_PATH": os.path.join(self.package_folder, "tapctl.exe").replace("\\", "/")
            })
