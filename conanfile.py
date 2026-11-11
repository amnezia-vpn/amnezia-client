from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "VirtualBuildEnv", "CMakeConfigDeps"

    options = {
        "macos_ne": [True, False]
    }
    default_options = {
        "macos_ne": False
    }

    def requirements(self):
        os = str(self.settings.os)
        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and not self.options.macos_ne)

        self.requires("libagw/1.0.8@amnezia")
        if has_service:
            self.requires("xtun/0.0.1@amnezia")
        self.requires("libssh/0.11.3@amnezia")
        self.requires("openssl/[>=1.1]@amnezia")
        self.requires("zlib/1.3.2")
