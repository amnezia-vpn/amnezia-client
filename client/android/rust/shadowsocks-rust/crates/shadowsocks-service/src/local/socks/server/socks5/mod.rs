//! SOCKS5 Local Server

pub use self::{tcprelay::Socks5TcpHandler, udprelay::Socks5UdpServer};

mod tcprelay;
mod udprelay;
