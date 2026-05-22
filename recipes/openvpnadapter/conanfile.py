from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.apple import is_apple_os
from conan.errors import ConanInvalidConfiguration
from conan.tools.scm import Git
from conan.internal.model.pkg_type import PackageType
from conan.tools.files import chdir

import os
import shutil
import shlex

class OpenVPNAdapter(ConanFile):
    name = "openvpnadapter"
    version = "1.0.0"
    settings = "os", "build_type"

    @property
    def _sdk(self):
        return str(self.settings.get_safe("os.sdk", "macosx"))

    @property
    def _platform(self):
        return {
            "macosx": "macOS",
            "iphoneos": "iOS",
            "iphonesimulator": "iOS Simulator"
        }.get(self._sdk)

    @property
    def _configuration(self):
        return "Debug" if self.settings.get_safe("build_type") == "Debug" else "Release"

    def layout(self):
        basic_layout(self)

    def validate(self):
        if not is_apple_os(self):
            raise ConanInvalidConfiguration(
                f"There is absolutely no point building Apple framework for {self.settings.os}"
            )

    def source(self):
        git = Git(self)
        git.clone(
            url="https://github.com/amnezia-vpn/OpenVPNAdapter.git",
            target=".",
            args=["--recurse-submodules", "--branch", "master-amnezia"]
        )

    def build(self):
        with chdir(self, self.source_folder):
            self.run("xcrun xcodebuild"
                " -project OpenVPNAdapter.xcodeproj"
                " -scheme OpenVPNAdapter"
                " -configuration Release"
                f" -destination 'generic/platform={self._platform}'"
                f" -sdk {self._sdk}"
                f' "CONFIGURATION_BUILD_DIR={self.build_folder}"'
                f' "BUILT_PRODUCTS_DIR={self.build_folder}"'
                " MACH_O_TYPE=staticlib"
                " BUILD_LIBRARY_FOR_DISTRIBUTION=YES"
                " CODE_SIGNING_ALLOWED=NO"
            )

    def package(self):
        package_framework = os.path.join(self.package_folder, "OpenVPNAdapter.framework")
        shutil.copytree(os.path.join(self.build_folder, "OpenVPNAdapter.framework"),
                        package_framework)

        framework_binary = os.path.join(package_framework, "OpenVPNAdapter")
        dependency_binaries = [
            os.path.join(self.build_folder, "OpenVPNClient.framework", "OpenVPNClient"),
            os.path.join(self.build_folder, "LZ4.framework", "LZ4"),
            os.path.join(self.build_folder, "mbedTLS.framework", "mbedTLS"),
        ]
        merged_binary = framework_binary + ".merged"
        self.run("xcrun libtool -static -o {} {}".format(
            shlex.quote(merged_binary),
            " ".join(shlex.quote(path) for path in [framework_binary] + dependency_binaries)
        ))
        shutil.move(merged_binary, framework_binary)

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "amnezia::openvpnadapter")
        self.cpp_info.type = PackageType.STATIC
        self.cpp_info.package_framework = True
        self.cpp_info.location = os.path.join(self.package_folder, "OpenVPNAdapter.framework")
        self.cpp_info.frameworks = ["SystemConfiguration"]
