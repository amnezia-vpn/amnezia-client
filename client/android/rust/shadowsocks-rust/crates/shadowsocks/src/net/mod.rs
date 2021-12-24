//! Network wrappers for shadowsocks' specific requirements

use std::net::SocketAddr;

pub use self::{
    option::{AcceptOpts, ConnectOpts},
    tcp::{TcpListener, TcpStream},
    udp::UdpSocket,
};

mod option;
pub mod tcp;
pub mod udp;

/// Address family `AF_INET`, `AF_INET6`
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum AddrFamily {
    /// `AF_INET`
    Ipv4,
    /// `AF_INET6`
    Ipv6,
}

impl From<&SocketAddr> for AddrFamily {
    fn from(addr: &SocketAddr) -> AddrFamily {
        match *addr {
            SocketAddr::V4(..) => AddrFamily::Ipv4,
            SocketAddr::V6(..) => AddrFamily::Ipv6,
        }
    }
}

impl From<SocketAddr> for AddrFamily {
    fn from(addr: SocketAddr) -> AddrFamily {
        match addr {
            SocketAddr::V4(..) => AddrFamily::Ipv4,
            SocketAddr::V6(..) => AddrFamily::Ipv6,
        }
    }
}
