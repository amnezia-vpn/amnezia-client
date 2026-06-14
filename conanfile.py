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

        has_ne = os == "iOS" or (os == "Macos" and self.options.macos_ne)
        has_service = os == "Windows" or os == "Linux" or (os == "Macos" and not has_ne)

        if has_service:
            if os == "Windows":
                self.requires("awg-windows/0.1.8")
                self.requires("tap-windows6/9.27.0")
                self.requires("win-split-tunnel/1.2.5.0")
                self.requires("wintun/0.14.1")
            else:
                self.requires("awg-go/0.2.16")

            self.requires("amnezia-xray-bindings/1.1.0")
            self.requires("tun2socks/2.6.0")
            self.requires("openvpn/2.7.0")
            self.requires("v2ray-rules-dat/202603162227")

        if has_ne:
            self.requires("awg-apple/2.0.1")
            self.requires("hev-socks5-tunnel/2.15.0", options={"as_framework": True})
            self.requires("openvpnadapter/1.0.0")

        if os == "Android":
            self.requires("amnezia-libxray/1.0.0")
            self.requires("awg-android/1.1.7")
            self.requires("openvpn-pt-android/1.0.0")

        # expicitly use libssh@amnezia to prevent it from being downloaded from conan-center
        self.requires("libssh/0.11.3@amnezia")
        self.requires("openssl/3.6.2")
        self.requires("zlib/1.3.2")
