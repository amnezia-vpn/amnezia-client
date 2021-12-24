//! UDP transparent proxy

use std::{
    io::{self, ErrorKind},
    net::{IpAddr, SocketAddr},
    sync::Arc,
    time::Duration,
};

use async_trait::async_trait;
use log::{error, info, trace, warn};
use shadowsocks::{
    lookup_then,
    relay::{socks5::Address, udprelay::MAXIMUM_UDP_PAYLOAD_SIZE},
    ServerAddr,
};

use crate::{
    config::RedirType,
    local::{
        context::ServiceContext,
        loadbalancing::PingBalancer,
        net::{UdpAssociationManager, UdpInboundWrite},
        redir::{
            redir_ext::{RedirSocketOpts, UdpSocketRedirExt},
            to_ipv4_mapped,
        },
    },
};

use self::sys::UdpRedirSocket;

mod sys;

#[derive(Clone)]
struct UdpRedirInboundWriter {
    redir_ty: RedirType,
    socket_opts: RedirSocketOpts,
}

#[async_trait]
impl UdpInboundWrite for UdpRedirInboundWriter {
    async fn send_to(&self, peer_addr: SocketAddr, remote_addr: &Address, data: &[u8]) -> io::Result<()> {
        let addr = match *remote_addr {
            Address::SocketAddress(sa) => {
                // Try to convert IPv4 mapped IPv6 address if server is running on dual-stack mode
                match sa {
                    SocketAddr::V4(..) => sa,
                    SocketAddr::V6(ref v6) => match to_ipv4_mapped(v6.ip()) {
                        Some(v4) => SocketAddr::new(IpAddr::from(v4), v6.port()),
                        None => sa,
                    },
                }
            }
            Address::DomainNameAddress(..) => {
                let err = io::Error::new(
                    ErrorKind::InvalidInput,
                    "redir destination must not be an domain name address",
                );
                return Err(err);
            }
        };

        // Create a socket binds to destination addr
        // This only works for systems that supports binding to non-local addresses
        //
        // This socket has to set SO_REUSEADDR and SO_REUSEPORT.
        // Outbound addresses could be connected from different source addresses.
        let inbound = UdpRedirSocket::bind_nonlocal(self.redir_ty, addr, &self.socket_opts)?;

        // Send back to client
        inbound.send_to(data, peer_addr).await.map(|n| {
            if n < data.len() {
                warn!(
                    "udp redir send back data (actual: {} bytes, sent: {} bytes), remote: {}, peer: {}",
                    n,
                    data.len(),
                    remote_addr,
                    peer_addr
                );
            }

            trace!(
                "udp redir send back data {} bytes, remote: {}, peer: {}, socket_opts: {:?}",
                n,
                remote_addr,
                peer_addr,
                self.socket_opts
            );
        })
    }
}

pub struct UdpRedir {
    context: Arc<ServiceContext>,
    redir_ty: RedirType,
    time_to_live: Option<Duration>,
    capacity: Option<usize>,
}

impl UdpRedir {
    pub fn new(
        context: Arc<ServiceContext>,
        redir_ty: RedirType,
        time_to_live: Option<Duration>,
        capacity: Option<usize>,
    ) -> UdpRedir {
        UdpRedir {
            context,
            redir_ty,
            time_to_live,
            capacity,
        }
    }

    pub async fn run(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let listener = match *client_config {
            ServerAddr::SocketAddr(ref saddr) => UdpRedirSocket::listen(self.redir_ty, *saddr)?,
            ServerAddr::DomainName(ref dname, port) => {
                lookup_then!(self.context.context_ref(), dname, port, |addr| {
                    UdpRedirSocket::listen(self.redir_ty, addr)
                })?
                .1
            }
        };

        let local_addr = listener.local_addr().expect("determine port bound to");
        info!(
            "shadowsocks UDP redirect ({}) listening on {}",
            self.redir_ty, local_addr
        );

        let manager = UdpAssociationManager::new(
            self.context.clone(),
            UdpRedirInboundWriter {
                redir_ty: self.redir_ty,
                socket_opts: RedirSocketOpts {
                    #[cfg(any(target_os = "linux", target_os = "android"))]
                    fwmark: self.context.connect_opts_ref().fwmark,

                    ..Default::default()
                },
            },
            self.time_to_live,
            self.capacity,
            balancer,
        );

        let mut pkt_buf = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        loop {
            let (recv_len, src, mut dst) = match listener.recv_dest_from(&mut pkt_buf).await {
                Ok(o) => o,
                Err(err) => {
                    error!("recv_dest_from failed with err: {}", err);
                    continue;
                }
            };

            // Packet length is limited by MAXIMUM_UDP_PAYLOAD_SIZE, excess bytes will be discarded.
            // Copy bytes, because udp_associate runs in another tokio Task
            let pkt = &pkt_buf[..recv_len];

            trace!(
                "received UDP packet from {}, destination {}, length {} bytes",
                src,
                dst,
                recv_len
            );

            if recv_len == 0 {
                // For windows, it will generate a ICMP Port Unreachable Message
                // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-recvfrom
                // Which will result in recv_from return 0.
                //
                // It cannot be solved here, because `WSAGetLastError` is already set.
                //
                // See `relay::udprelay::utils::create_socket` for more detail.
                continue;
            }

            // Try to convert IPv4 mapped IPv6 address for dual-stack mode.
            if let SocketAddr::V6(ref a) = dst {
                if let Some(v4) = to_ipv4_mapped(a.ip()) {
                    dst = SocketAddr::new(IpAddr::from(v4), a.port());
                }
            }

            if let Err(err) = manager.send_to(src, Address::from(dst), pkt).await {
                error!(
                    "udp packet relay {} -> {} with {} bytes failed, error: {}",
                    src,
                    dst,
                    pkt.len(),
                    err
                );
            }
        }
    }
}
