//! Shadowsocks Service
//!
//! <https://shadowsocks.org/>
//!
//! shadowsocks is a fast tunnel proxy that helps you bypass firewalls.
//!
//! ## Usage
//!
//! Build shadowsocks and you will get at least 2 binaries: `sslocal` and `ssserver`
//!
//! Write your servers in a configuration file. Format is defined in
//! [shadowsocks' documentation](https://github.com/shadowsocks/shadowsocks/wiki)
//!
//! For example:
//!
//! ```json
//! {
//!    "server": "my_server_ip",
//!    "server_port": 8388,
//!    "local_address": "127.0.0.1",
//!    "local_port": 1080,
//!    "password": "mypassword",
//!    "timeout": 300,
//!    "method": "aes-256-cfb"
//! }
//! ```
//!
//! Save it in file `shadowsocks.json` and run local proxy server with
//!
//! ```bash
//! cargo run --bin sslocal -- -c shadowsocks.json
//! ```
//!
//! Now you can use SOCKS5 protocol to proxy your requests, for example:
//!
//! ```bash
//! curl --socks5-hostname 127.0.0.1:1080 https://www.google.com
//! ```
//!
//! On the server side, you can run the server with
//!
//! ```bash
//! cargo run --bin ssserver -- -c shadowsocks.json
//! ```
//!
//! Server should use the same configuration file as local, except the listen addresses for servers must be socket
//! addresses.
//!
//! Of course, you can also use `cargo install` to install binaries.

use std::time::Duration;

#[cfg(feature = "local")]
pub use self::local::run as run_local;
#[cfg(feature = "manager")]
pub use self::manager::run as run_manager;
#[cfg(feature = "server")]
pub use self::server::run as run_server;
pub use shadowsocks;

pub mod acl;
pub mod config;
mod dns;
#[cfg(feature = "local")]
pub mod local;
#[cfg(feature = "manager")]
pub mod manager;
pub mod net;
#[cfg(feature = "server")]
pub mod server;
mod sys;

/// Default UDP association's expire duration
#[allow(dead_code)]
const DEFAULT_UDP_EXPIRY_DURATION: Duration = Duration::from_secs(5 * 60);

#[cfg(feature = "trust-dns")]
fn hint_support_default_system_resolver() -> bool {
    // Nearly all *nix system have /etc/resolv.conf, except Android.
    // macOS have to use system provided resolver.
    cfg!(all(
        unix,
        not(target_os = "android"),
        /* not(target_os = "macos"),
         * not(target_os = "ios") */
    ))
}
