from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "VirtualBuildEnv", "CMakeConfigDeps"

    def requirements(self):
        self.requires("libagw/1.0.8@amnezia")
        self.requires("xtun/1.0.0@amnezia")
        self.requires("libssh/0.11.3@amnezia")
        self.requires("openssl/[>=1.1]@amnezia")
        self.requires("zlib/1.3.2")
