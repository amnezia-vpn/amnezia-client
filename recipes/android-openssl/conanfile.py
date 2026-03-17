import os
import platform
import re
import shutil

from conan import ConanFile
from conan.tools.files import copy
from conan.tools.scm import Git
from conan.errors import ConanInvalidConfiguration, ConanException


class AndroidOpenSSL(ConanFile):
    name = "android-openssl"
    version = "3.5.5"
    settings = "os", "arch"
    options = {"page_16k": [True, False]}
    default_options = {"page_16k": True}

    # Conan arch → OpenSSL Configure target
    _arch_map = {
        "armv8":   "android-arm64",
        "armv7":   "android-arm",
        "x86_64":  "android-x86_64",
        "x86":     "android-x86",
    }

    def validate(self):
        if self.settings.os != "Android":
            raise ConanInvalidConfiguration(f"{self.name} only supports Android")
        if str(self.settings.arch) not in self._arch_map:
            raise ConanInvalidConfiguration(f"Unsupported arch: {self.settings.arch}")

    def source(self):
        git = Git(self)
        git.clone(
            url="https://github.com/openssl/openssl.git",
            target=".",
            args=["--branch", f"openssl-{self.version}", "--depth", "1"]
        )

    def _ndk(self):
        ndk = self.conf.get("tools.android:ndk_path", check_type=str) or \
              os.environ.get("ANDROID_NDK_ROOT", "")
        if not ndk:
            raise ConanException(
                "Android NDK not found. Set tools.android:ndk_path conf or ANDROID_NDK_ROOT env."
            )
        return ndk

    def _toolchain_bin(self):
        host = "linux-x86_64" if platform.system() == "Linux" else "darwin-x86_64"
        return os.path.join(self._ndk(), "toolchains", "llvm", "prebuilt", host, "bin")

    def _patch_makefile(self, makefile_path):
        """Rename libcrypto.so → libcrypto_3.so and libssl.so → libssl_3.so
        throughout the generated Makefile so the built files already carry the
        correct names and sonames — no patchelf required."""
        with open(makefile_path, "r") as f:
            content = f.read()

        # Replace every standalone libcrypto.so / libssl.so reference.
        # \b word-boundary keeps us from matching libcrypto.so.3 style names.
        content = re.sub(r"\blibcrypto\.so\b", "libcrypto_3.so", content)
        content = re.sub(r"\blibssl\.so\b",    "libssl_3.so",    content)

        with open(makefile_path, "w") as f:
            f.write(content)

    def build(self):
        ndk = self._ndk()
        toolchain_bin = self._toolchain_bin()
        target = self._arch_map[str(self.settings.arch)]
        api_level = str(self.settings.os.api_level)
        jobs = os.cpu_count() or 4

        env_prefix = f'PATH="{toolchain_bin}:$PATH" ANDROID_NDK_ROOT="{ndk}"'

        configure_cmd = (
            f"./Configure {target} shared no-tests"
            f" -D__ANDROID_API__={api_level}"
        )
        if self.options.page_16k:
            configure_cmd += " -Wl,-z,max-page-size=16384"

        self.run(f"{env_prefix} {configure_cmd}", cwd=self.source_folder)

        # Bake the _3 names into the Makefile before building so the output
        # files are already libcrypto_3.so / libssl_3.so with correct sonames
        # and DT_NEEDED entries — no patchelf step required.
        self._patch_makefile(os.path.join(self.source_folder, "Makefile"))

        self.run(
            f"{env_prefix} make -j{jobs} SHLIB_VERSION_NUMBER= build_libs",
            cwd=self.source_folder
        )

    def package(self):
        lib_dst = os.path.join(self.package_folder, "lib")
        os.makedirs(lib_dst, exist_ok=True)

        copy(self, "libcrypto_3.so", src=self.source_folder, dst=lib_dst, keep_path=False)
        copy(self, "libssl_3.so",    src=self.source_folder, dst=lib_dst, keep_path=False)

        # Static libs (still named without suffix)
        copy(self, "libcrypto.a", src=self.source_folder, dst=lib_dst, keep_path=False)
        copy(self, "libssl.a",    src=self.source_folder, dst=lib_dst, keep_path=False)

        # Headers
        copy(self, "include/openssl/*.h", src=self.source_folder, dst=self.package_folder)

    def package_info(self):
        # Expose the same CMake targets as the upstream openssl package so
        # consumers (libssh, main app) need no changes to their target_link_libraries.
        self.cpp_info.components["crypto"].set_property("cmake_target_name", "OpenSSL::Crypto")
        self.cpp_info.components["crypto"].libs = ["crypto_3"]
        self.cpp_info.components["crypto"].includedirs = ["include"]
        self.cpp_info.components["crypto"].libdirs = ["lib"]

        self.cpp_info.components["ssl"].set_property("cmake_target_name", "OpenSSL::SSL")
        self.cpp_info.components["ssl"].libs = ["ssl_3"]
        self.cpp_info.components["ssl"].includedirs = []
        self.cpp_info.components["ssl"].libdirs = ["lib"]
        self.cpp_info.components["ssl"].requires = ["crypto"]
