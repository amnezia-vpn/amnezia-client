from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.files import copy, replace_in_file
from conan.tools.env import VirtualBuildEnv, Environment
from conan.errors import ConanInvalidConfiguration
from conan.tools.scm import Git

import os
import platform
from pathlib import Path

class AwgAndroid(ConanFile):
    name = "awg-android"
    version = "3.1.20260814"
    settings = "os", "arch", "build_type", "compiler"

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.4.1 <4]")
        if platform.system() == "Windows":
            self.tool_requires("ninja/[*]")
            self.tool_requires("go/[*]")
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} v{self.version} does not support {self.settings.os}")

    def source(self):
        git = Git(self)
        git.clone(
            url="https://github.com/amnezia-vpn/amneziawg-android.git",
            target=".",
            args=["--recurse-submodules", "--branch", f"v{self.version}"]
        )

    def generate(self):
        VirtualBuildEnv(self).generate()

        tc = CMakeToolchain(self)
        tc.variables["GRADLE_USER_HOME"] = Path(os.path.join(self.build_folder, "gradle_user_home")).as_posix()
        tc.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = Path(os.path.join(self.build_folder, "out")).as_posix()
        # not to warn in case of strtok() usage
        tc.extra_cflags = ["-Wno-deprecated-declarations"]
        tc.generate()

    def _patch_sources(self):
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
            # elf-cleaner uses sys/mman.h (POSIX only) and cannot be built on Windows;
            # skip it — DT_FLAGS_1 warnings only affect Android < 6.0
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "CMakeLists.txt"),
                '# Strip unwanted ELF sections to prevent DT_FLAGS_1 warnings on old Android versions\n'
                'file(GLOB ELF_CLEANER_SOURCES elf-cleaner/*.c elf-cleaner/*.cpp)\n'
                'add_custom_target(elf-cleaner COMMENT "Building elf-cleaner" VERBATIM COMMAND cc\n'
                '        -O2 -DPACKAGE_NAME="elf-cleaner" -DPACKAGE_VERSION="" -DCOPYRIGHT=""\n'
                '        -o "${CMAKE_CURRENT_BINARY_DIR}/elf-cleaner" ${ELF_CLEANER_SOURCES}\n'
                ')\n'
                'add_custom_command(TARGET libwg.so POST_BUILD VERBATIM COMMAND "${CMAKE_CURRENT_BINARY_DIR}/elf-cleaner"\n'
                '        --api-level "${ANDROID_NATIVE_API_LEVEL}" "$<TARGET_FILE:libwg.so>")\n'
                'add_dependencies(libwg.so elf-cleaner)\n'
                'add_custom_command(TARGET libwg-quick.so POST_BUILD VERBATIM COMMAND "${CMAKE_CURRENT_BINARY_DIR}/elf-cleaner"\n'
                '        --api-level "${ANDROID_NATIVE_API_LEVEL}" "$<TARGET_FILE:libwg-quick.so>")\n'
                'add_dependencies(libwg-quick.so elf-cleaner)',
                '',
            )
            # patch Makefile: skip Go download, use 'go' already in PATH from tool_requires
            replace_in_file(self,
                os.path.join(self.source_folder, "tunnel", "tools", "libwg-go", "Makefile"),
                '$(DESTDIR)/libwg-go.so: export PATH := $(BUILDDIR)/go-$(GO_VERSION)/bin/:$(PATH)\n$(DESTDIR)/libwg-go.so: $(BUILDDIR)/go-$(GO_VERSION)/.prepared go.mod',
                '$(DESTDIR)/libwg-go.so: go.mod',
            )

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
