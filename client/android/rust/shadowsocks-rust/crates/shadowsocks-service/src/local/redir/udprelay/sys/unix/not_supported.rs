use std::{io, net::SocketAddr};

use crate::{
    config::RedirType,
    local::redir::redir_ext::{RedirSocketOpts, UdpSocketRedir},
};

pub struct UdpRedirSocket;

impl UdpRedirSocket {
    /// Create a new UDP socket binded to `addr`
    ///
    /// This will allow listening to `addr` that is not in local host
    pub fn listen(_ty: RedirType, _addr: SocketAddr) -> io::Result<UdpRedirSocket> {
        unimplemented!("UDP transparent proxy is not supported on this platform")
    }

    /// Create a new UDP socket binded to `addr`
    ///
    /// This will allow binding to `addr` that is not in local host
    pub fn bind_nonlocal(
        _ty: RedirType,
        _addr: SocketAddr,
        _redir_opts: &RedirSocketOpts,
    ) -> io::Result<UdpRedirSocket> {
        unimplemented!("UDP transparent proxy is not supported on this platform")
    }

    /// Send data to the socket to the given target address
    pub async fn send_to(&self, _buf: &[u8], _target: SocketAddr) -> io::Result<usize> {
        unimplemented!("UDP transparent proxy is not supported on this platform")
    }

    /// Returns the local address that this socket is bound to.
    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        unimplemented!("UDP transparent proxy is not supported on this platform")
    }
}

impl UdpSocketRedir for UdpRedirSocket {
    fn poll_recv_dest_from(
        &self,
        _cx: &mut Context<'_>,
        _buf: &mut [u8],
    ) -> Poll<io::Result<(usize, SocketAddr, SocketAddr)>> {
        unimplemented!("UDP transparent proxy is not supported on this platform")
    }
}
