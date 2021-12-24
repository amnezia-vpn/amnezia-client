//! Shadowsocks SOCKS (4/4a, 5) Local Server

pub use self::server::Socks;

pub mod client;
pub mod server;
#[cfg(feature = "local-socks4")]
pub mod socks4;
