from conan import ConanFile
from conan.tools.files import get, copy
from conan.tools.layout import basic_layout
from conan.errors import ConanInvalidConfiguration

import os


class LlvmMingw(ConanFile):
    name = "llvm-mingw"
    version = "20260616"
    settings = "os", "arch"
    package_type = "application"

    # The flavour is picked by the machine this tool package runs on (the
    # conan build profile); every flavour targets all four
    # {i686,x86_64,armv7,aarch64}-w64-mingw32 triples, so a single package
    # serves both native ARM64 hosts and x64 CI runners cross-building arm64.
    @property
    def _host_flavour(self):
        return {
            "x86_64": "x86_64",
            "armv8": "aarch64",
        }.get(str(self.settings.arch))

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not str(self.settings.os).startswith("Windows"):
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} is to be used on Windows only!"
            )
        if not self._host_flavour:
            raise ConanInvalidConfiguration(
                f"{self.name} v{self.version} has no toolchain build for a {self.settings.arch} machine"
            )

    def build(self):
        # settings-dependent source, so fetched in build() rather than source()
        get(self, **self.conan_data["sources"][self.version][self._host_flavour], strip_root=True)

    def package(self):
        for subdir in ("bin", "include", "lib", "share", "generic-w64-mingw32",
                       "i686-w64-mingw32", "x86_64-w64-mingw32",
                       "armv7-w64-mingw32", "aarch64-w64-mingw32"):
            copy(self, "*", src=os.path.join(self.build_folder, subdir),
                 dst=os.path.join(self.package_folder, subdir))

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
        self.buildenv_info.prepend_path("PATH", os.path.join(self.package_folder, "bin"))
