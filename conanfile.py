from conan import ConanFile

class AmneziaVPN(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeConfigDeps"

    options = {
        "macos_ne": [True, False]
    }
    default_options = {
        "macos_ne": False
    }

    def requirements(self):
        os = str(self.settings.os)

        has_ne = os == "iOS" or (os == "Macos" and self.options.macos_ne)
        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and not has_ne)

        if has_service:
            if os == "Windows":
                self.requires("awg-windows/1.0.0")
            else:
                self.requires("awg-go/0.2.16")

            self.requires("amnezia-xray-bindings/1.1.0")
            self.requires("tun2socks/2.6.0")
            self.requires("openvpn/2.7.0")
            self.requires("v2ray-rules-dat/[*]")

        if has_ne:
            self.requires("awg-apple/2.0.1")
            self.requires("hev-socks5-tunnel/2.14.4", options={"as_framework": True})
            self.requires("openvpnadapter/1.0.0")

        if os == "Android":
            self.requires("amnezia-libxray/1.0.0")
            self.requires("awg-android/1.1.7")
            self.requires("openvpn-pt-android/1.0.0")

        self.requires("libssh/0.11.3@amnezia")
        self.requires("openssl/3.5.5")
