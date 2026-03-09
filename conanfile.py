from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeConfigDeps"

    def requirements(self):
        os = str(self.settings.os)
        arch = str(self.settings.arch)

        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and arch == "x86_64")
        has_network_extension = os == "iOS" or (os == "Macos" and not has_service)

        if has_service:
            if os == "Windows":
                self.requires("awg-windows/1.0.0")
            else:
                self.requires("awg-go/0.2.16")

            self.requires("amnezia-xray-bindings/1.1.0")
            self.requires("tun2socks/2.6.0")
            self.requires("openvpn/2.7.0")

        if has_network_extension:
            self.requires("awg-apple/2.0.1")

        if os == "Android":
            self.requires("amnezia-libxray/1.0.0")
            self.requires("awg-android/1.1.7")
            self.requires("openvpn-pt-android/1.0.0")

        self.requires("libssh-local/0.11.3")
        self.requires("openssl/3.5.5")
