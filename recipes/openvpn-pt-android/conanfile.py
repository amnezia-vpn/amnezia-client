from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.files import copy, replace_in_file
from conan.tools.scm import Git
from conan.errors import ConanInvalidConfiguration

import os

class OpenvpnPtAndroid(ConanFile):
    name = "openvpn-pt-android"
    version = "1.0.0"
    package_type = "shared-library"
    settings = "os", "arch", "build_type", "compiler"

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
        tc.generate()

    def _patch_sources(self):
        replace_in_file(self,
            os.path.join(self.source_folder, "cloak.cmake"),
            "COMMAND ${GO_EXEC} ${BUILD_CMD_ARGS}",
            "COMMAND ${CMAKE_COMMAND} -E env CGO_CFLAGS=${CMAKE_C_FLAGS} CGO_LDFLAGS=${CMAKE_SHARED_LINKER_FLAGS} ${GO_EXEC} ${BUILD_CMD_ARGS}",
        )

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure()
        cmake.build(target=["ck_ovpn_plugin_go", "ovpn3", "ovpnutil", "rsapss"])

    def package(self):
        copy(self, "*.h", src=self.build_folder, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.so", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::openvpn-pt-android")
        self.cpp_info.libs = [ "ovpn3", "ovpnutil", "rsapss" ]
        self.cpp_info.set_property("cmake_extra_variables", {
            "OPENVPN_PT_ANDROID_LIBCK_OVPN_PLUGIN_PATH": os.path.join(self.package_folder, "lib", "libck-ovpn-plugin.so")
        })
