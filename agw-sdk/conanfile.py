from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class AgwSdkConan(ConanFile):
    name = "agw-sdk"
    version = "0.1.0"
    license = "TBD"
    description = "AGW SDK — Qt-free C++ transport to the Amnezia API gateway (Tier 1)"
    settings = "os", "compiler", "build_type", "arch"

    # shared-deps: линкуем общий OpenSSL/nlohmann из Conan (наши приложения).
    # vendored: бандлим зависимости со скрытием символов (сторонние/standalone) — Фаза 5.
    options = {
        "deps_mode": ["shared-deps", "vendored"],
        "build_tests": [True, False],
    }
    default_options = {
        "deps_mode": "shared-deps",
        "build_tests": False,
    }

    exports_sources = "CMakeLists.txt", "include/*", "src/*", "tests/*"

    def requirements(self):
        # Версия OpenSSL совпадает с приложением (см. корневой conanfile.py) — без второго OpenSSL.
        self.requires("openssl/3.6.2")
        self.requires("nlohmann_json/3.11.3")
        # libcurl добавляется в Фазе 2 (за IHttpClient).

    def build_requirements(self):
        if self.options.build_tests:
            self.test_requires("gtest/1.15.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["AGW_DEPS_MODE"] = str(self.options.deps_mode)
        tc.variables["AGW_BUILD_TESTS"] = bool(self.options.build_tests)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["agw"]
        self.cpp_info.includedirs = ["include"]
