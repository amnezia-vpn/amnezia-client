//! SOCKS clients

#[cfg(feature = "local-socks4")]
pub use self::socks4::Socks4TcpClient;
pub use self::socks5::{Socks5TcpClient, Socks5UdpClient};

#[cfg(feature = "local-socks4")]
pub mod socks4;
pub mod socks5;
