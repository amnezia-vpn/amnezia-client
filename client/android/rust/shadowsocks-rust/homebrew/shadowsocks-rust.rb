class ShadowsocksRust < Formula
    desc "shadowsocks is a fast tunnel proxy that helps you bypass firewalls"
    homepage "https://github.com/shadowsocks/shadowsocks-rust"
    url "https://github.com/shadowsocks/shadowsocks-rust/archive/refs/tags/v1.10.3.tar.gz"
    version "1.10.3"
    sha256 "00fb90b6f80d01c6b40f6cfeb49d70fbec9f659bfa268d6834e79fe1f670d55e"
    license "MIT"

    head do
        url "https://github.com/shadowsocks/shadowsocks-rust.git"
    end
  
    depends_on "rust" => :build
  
    def install
      ENV.with_build_environment do
        ENV["RUSTFLAGS"] = "-C target-cpu=native"
        system "cargo", "install", *std_cargo_args
      end
    end
  
    test do
      system bin/"sslocal", "--help"
      system bin/"ssserver", "--help"
      system bin/"ssmanager", "--help"
    end
  end
