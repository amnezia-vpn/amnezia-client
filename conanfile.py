from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        os = str(self.settings.os)
        arch = str(self.settings.arch)

        if os == "Windows" or os == "Linux" or (os == "Macos" and arch == "x86_64"):
            self.requires("amnezia-xray-bindings/1.1.0")
            self.requires("awg-go/0.2.16")

        if os == "iOS" or (os == "Macos" and arch == "arm64"):
            self.requires("awg-apple/2.0.1")
