from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy, collect_libs
from conan.tools.scm import Git
from conan.errors import ConanInvalidConfiguration

import os

class OpenvpnPtAndroid(ConanFile):
    name = "openvpn-pt-android"
    version = "1.0.0"
    package_type = "shared-library"
    settings = "os", "arch", "build_type", "compiler"
    options = {"page_16k": [True, False]}
    default_options = {"page_16k": True}

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("swig/4.1.1")
        self.tool_requires("go/1.26.0")
        self.tool_requires("cmake/[>=3.4.1 <4]")

    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} only supports Android, got {self.settings.os}")

    def source(self):
        git = Git(self)
        git.clone(
            url="https://github.com/amnezia-vpn/openvpn-pt-android.git",
            target=".",
            args=["--recurse-submodules", "--branch", "update-ovpn3"]
        )

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ANDROID_PACKAGE_NAME"] = "org.amnezia.vpn"
        tc.variables["ANDROID_PLATFORM"] = 24
        if self.options.page_16k:
            tc.extra_ldflags = ["-Wl,-z,max-page-size=16384"]
            tc.cache_variables["CMAKE_SHARED_LINKER_FLAGS"] = "-Wl,-z,max-page-size=16384"
            tc.cache_variables["CMAKE_EXE_LINKER_FLAGS"] = "-Wl,-z,max-page-size=16384"
        tc.generate()

        vbe = VirtualBuildEnv(self)
        if self.options.page_16k:
            vbe.environment().define("CGO_LDFLAGS", "-Wl,-z,max-page-size=16384")
        vbe.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build(target=["ck_ovpn_plugin_go", "ovpn3", "ovpnutil", "rsapss"])

    def package(self):
        copy(self, "*.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.so", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::openvpn-pt-android")
        self.cpp_info.libs = collect_libs(self)
