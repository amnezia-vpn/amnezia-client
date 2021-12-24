//! UDP socket for communicating with shadowsocks' proxy server

use std::{io, net::SocketAddr, time::Duration};

use bytes::BytesMut;
use log::{trace, warn};
use once_cell::sync::Lazy;
use tokio::{
    net::{ToSocketAddrs, UdpSocket},
    time,
};

use crate::{
    config::{ServerAddr, ServerConfig},
    context::SharedContext,
    crypto::v1::CipherKind,
    net::{ConnectOpts, UdpSocket as OutboundUdpSocket},
    relay::socks5::Address,
};

use super::crypto_io::{decrypt_payload, encrypt_payload};

static DEFAULT_CONNECT_OPTS: Lazy<ConnectOpts> = Lazy::new(Default::default);

/// UDP client for communicating with ShadowSocks' server
pub struct ProxySocket {
    socket: UdpSocket,
    method: CipherKind,
    key: Box<[u8]>,
    send_timeout: Option<Duration>,
    recv_timeout: Option<Duration>,
    context: SharedContext,
}

impl ProxySocket {
    /// Create a client to communicate with Shadowsocks' UDP server
    pub async fn connect(context: SharedContext, svr_cfg: &ServerConfig) -> io::Result<ProxySocket> {
        ProxySocket::connect_with_opts(context, svr_cfg, &DEFAULT_CONNECT_OPTS).await
    }

    /// Create a client to communicate with Shadowsocks' UDP server
    pub async fn connect_with_opts(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        opts: &ConnectOpts,
    ) -> io::Result<ProxySocket> {
        // Note: Plugins doesn't support UDP relay

        let socket = OutboundUdpSocket::connect_server_with_opts(&context, svr_cfg.addr(), opts).await?;

        trace!("connected udp remote {} with {:?}", svr_cfg.addr(), opts);

        Ok(ProxySocket::from_socket(context, svr_cfg, socket.into()))
    }

    /// Create a `ProxySocket` from a `UdpSocket`
    pub fn from_socket(context: SharedContext, svr_cfg: &ServerConfig, socket: UdpSocket) -> ProxySocket {
        let key = svr_cfg.key().to_vec().into_boxed_slice();
        let method = svr_cfg.method();

        // NOTE: svr_cfg.timeout() is not for this socket, but for associations.

        ProxySocket {
            socket,
            method,
            key,
            send_timeout: None,
            recv_timeout: None,
            context,
        }
    }

    /// Create a `ProxySocket` binding to a specific address
    pub async fn bind(context: SharedContext, svr_cfg: &ServerConfig) -> io::Result<ProxySocket> {
        // Plugins doesn't support UDP
        let socket = match svr_cfg.addr() {
            ServerAddr::SocketAddr(sa) => UdpSocket::bind(sa).await?,
            ServerAddr::DomainName(domain, port) => UdpSocket::bind((domain.as_str(), *port)).await?,
        };
        Ok(ProxySocket::from_socket(context, svr_cfg, socket))
    }

    /// Send a UDP packet to addr through proxy
    pub async fn send(&self, addr: &Address, payload: &[u8]) -> io::Result<usize> {
        let mut send_buf = BytesMut::new();
        encrypt_payload(&self.context, self.method, &self.key, addr, payload, &mut send_buf);

        trace!(
            "UDP server client send to {}, payload length {} bytes, packet length {} bytes",
            addr,
            payload.len(),
            send_buf.len()
        );

        let send_len = match self.send_timeout {
            None => self.socket.send(&send_buf).await?,
            Some(d) => match time::timeout(d, self.socket.send(&send_buf)).await {
                Ok(Ok(l)) => l,
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(io::ErrorKind::TimedOut.into()),
            },
        };

        if send_buf.len() != send_len {
            warn!(
                "UDP server client send {} bytes, but actually sent {} bytes",
                send_buf.len(),
                send_len
            );
        }

        Ok(send_len)
    }

    /// Send a UDP packet to target from proxy
    pub async fn send_to<A: ToSocketAddrs>(&self, target: A, addr: &Address, payload: &[u8]) -> io::Result<usize> {
        let mut send_buf = BytesMut::new();
        encrypt_payload(&self.context, self.method, &self.key, addr, payload, &mut send_buf);

        trace!(
            "UDP server client send to, addr {}, payload length {} bytes, packet length {} bytes",
            addr,
            payload.len(),
            send_buf.len()
        );

        let send_len = match self.send_timeout {
            None => self.socket.send_to(&send_buf, target).await?,
            Some(d) => match time::timeout(d, self.socket.send_to(&send_buf, target)).await {
                Ok(Ok(l)) => l,
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(io::ErrorKind::TimedOut.into()),
            },
        };

        if send_buf.len() != send_len {
            warn!(
                "UDP server client send {} bytes, but actually sent {} bytes",
                send_buf.len(),
                send_len
            );
        }

        Ok(send_len)
    }

    /// Receive packet from Shadowsocks' UDP server
    ///
    /// This function will use `recv_buf` to store intermediate data, so it has to be big enough to store the whole shadowsocks' packet
    ///
    /// It is recommended to allocate a buffer to have at least 65536 bytes.
    pub async fn recv(&self, recv_buf: &mut [u8]) -> io::Result<(usize, Address, usize)> {
        // Waiting for response from server SERVER -> CLIENT
        let recv_n = match self.recv_timeout {
            None => self.socket.recv(recv_buf).await?,
            Some(d) => match time::timeout(d, self.socket.recv(recv_buf)).await {
                Ok(Ok(l)) => l,
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(io::ErrorKind::TimedOut.into()),
            },
        };

        let (n, addr) = decrypt_payload(&self.context, self.method, &self.key, &mut recv_buf[..recv_n]).await?;

        trace!(
            "UDP server client receive from {}, packet length {} bytes, payload length {} bytes",
            addr,
            recv_n,
            n
        );

        Ok((n, addr, recv_n))
    }

    /// Receive packet from Shadowsocks' UDP server
    ///
    /// This function will use `recv_buf` to store intermediate data, so it has to be big enough to store the whole shadowsocks' packet
    ///
    /// It is recommended to allocate a buffer to have at least 65536 bytes.
    pub async fn recv_from(&self, recv_buf: &mut [u8]) -> io::Result<(usize, SocketAddr, Address, usize)> {
        // Waiting for response from server SERVER -> CLIENT
        let (recv_n, target_addr) = match self.recv_timeout {
            None => self.socket.recv_from(recv_buf).await?,
            Some(d) => match time::timeout(d, self.socket.recv_from(recv_buf)).await {
                Ok(Ok(l)) => l,
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(io::ErrorKind::TimedOut.into()),
            },
        };
        let (n, addr) = decrypt_payload(&self.context, self.method, &self.key, &mut recv_buf[..recv_n]).await?;

        trace!(
            "UDP server client receive from {}, addr {}, packet length {} bytes, payload length {} bytes",
            target_addr,
            addr,
            recv_n,
            n,
        );

        Ok((n, target_addr, addr, recv_n))
    }

    /// Get local addr of socket
    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.socket.local_addr()
    }

    /// Set `send` timeout, `None` will clear timeout
    pub fn set_send_timeout(&mut self, t: Option<Duration>) {
        self.send_timeout = t;
    }

    /// Set `recv` timeout, `None` will clear timeout
    pub fn set_recv_timeout(&mut self, t: Option<Duration>) {
        self.recv_timeout = t;
    }
}
