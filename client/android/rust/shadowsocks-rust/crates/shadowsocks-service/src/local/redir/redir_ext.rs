//! Extension trait for `TcpListener` and `UdpSocket`

use std::{
    future::Future,
    io,
    net::SocketAddr,
    pin::Pin,
    task::{Context, Poll},
};

use async_trait::async_trait;
use tokio::net::TcpListener;

use crate::config::RedirType;

/// Extension function for `TcpListener` for setting extra options before `bind()`
#[async_trait]
pub trait TcpListenerRedirExt {
    // Create a TcpListener for transparent proxy
    //
    // Implementation is platform dependent
    async fn bind_redir(ty: RedirType, addr: SocketAddr) -> io::Result<TcpListener>;
}

/// Extension function for `TcpStream` for reading original destination address
pub trait TcpStreamRedirExt {
    // Read destination address for TcpStream
    //
    // Implementation is platform dependent
    fn destination_addr(&self, ty: RedirType) -> io::Result<SocketAddr>;
}

/// `UdpSocket` that support transparent proxy
pub trait UdpSocketRedir {
    /// Receive a single datagram from the socket.
    ///
    /// On success, the future resolves to the number of bytes read and the source, target address
    ///
    /// `(bytes read, source address, target address)`
    fn poll_recv_dest_from(
        &self,
        cx: &mut Context<'_>,
        buf: &mut [u8],
    ) -> Poll<io::Result<(usize, SocketAddr, SocketAddr)>>;
}

/// Extension functions for `UdpSocket` to receive data with original destination address
pub trait UdpSocketRedirExt {
    fn recv_dest_from<'a>(&'a self, buf: &'a mut [u8]) -> RecvDestFrom<'a, Self>
    where
        Self: Sized,
    {
        RecvDestFrom { socket: self, buf }
    }
}

impl<S> UdpSocketRedirExt for S where S: UdpSocketRedir {}

/// Future for `recv_dest_from`
pub struct RecvDestFrom<'a, S: 'a> {
    socket: &'a S,
    buf: &'a mut [u8],
}

impl<'a, S: 'a> Future for RecvDestFrom<'a, S>
where
    S: UdpSocketRedir,
{
    type Output = io::Result<(usize, SocketAddr, SocketAddr)>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        self.socket.poll_recv_dest_from(cx, self.buf)
    }
}

// sockopts for send-back sockets
#[derive(Debug, Clone)]
pub struct RedirSocketOpts {
    /// Linux mark based routing, going to set by `setsockopt` with `SO_MARK` option
    #[cfg(any(target_os = "linux", target_os = "android"))]
    pub fwmark: Option<u32>,
}

impl Default for RedirSocketOpts {
    fn default() -> RedirSocketOpts {
        RedirSocketOpts {
            #[cfg(any(target_os = "linux", target_os = "android"))]
            fwmark: None,
        }
    }
}
