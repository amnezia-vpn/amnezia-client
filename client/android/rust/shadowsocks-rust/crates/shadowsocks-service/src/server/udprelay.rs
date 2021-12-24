//! Shadowsocks UDP server

use std::{io, net::SocketAddr, sync::Arc, time::Duration};

use bytes::Bytes;
use futures::future::{self, AbortHandle};
use io::ErrorKind;
use log::{debug, error, info, trace, warn};
use lru_time_cache::{Entry, LruCache};
use shadowsocks::{
    lookup_then,
    net::UdpSocket as OutboundUdpSocket,
    relay::{
        socks5::Address,
        udprelay::{ProxySocket, MAXIMUM_UDP_PAYLOAD_SIZE},
    },
    ServerConfig,
};
use spin::Mutex as SpinMutex;
use tokio::{
    sync::{mpsc, Mutex},
    time,
};

use crate::net::MonProxySocket;

use super::context::ServiceContext;

pub struct UdpServer {
    context: Arc<ServiceContext>,
    assoc_map: Arc<Mutex<LruCache<SocketAddr, UdpAssociation>>>,
    cleanup_abortable: AbortHandle,
}

impl Drop for UdpServer {
    fn drop(&mut self) {
        self.cleanup_abortable.abort();
    }
}

impl UdpServer {
    pub fn new(context: Arc<ServiceContext>, time_to_live: Option<Duration>, capacity: Option<usize>) -> UdpServer {
        let time_to_live = time_to_live.unwrap_or(crate::DEFAULT_UDP_EXPIRY_DURATION);
        let assoc_map = Arc::new(Mutex::new(match capacity {
            Some(capacity) => LruCache::with_expiry_duration_and_capacity(time_to_live, capacity),
            None => LruCache::with_expiry_duration(time_to_live),
        }));

        let cleanup_abortable = {
            let assoc_map = assoc_map.clone();
            let (cleanup_task, cleanup_abortable) = future::abortable(async move {
                loop {
                    time::sleep(time_to_live).await;

                    // iter() will trigger a cleanup of expired associations
                    let _ = assoc_map.lock().await.iter();
                }
            });
            tokio::spawn(cleanup_task);
            cleanup_abortable
        };

        UdpServer {
            context,
            assoc_map,
            cleanup_abortable,
        }
    }

    pub async fn run(mut self, svr_cfg: &ServerConfig) -> io::Result<()> {
        let socket = ProxySocket::bind(self.context.context(), svr_cfg).await?;

        info!(
            "shadowsocks udp server listening on {}",
            socket.local_addr().expect("listener.local_addr"),
        );

        let socket = MonProxySocket::from_socket(socket, self.context.flow_stat());
        let listener = Arc::new(socket);

        let mut buffer = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        loop {
            let (n, peer_addr, target_addr) = match listener.recv_from(&mut buffer).await {
                Ok(s) => s,
                Err(err) => {
                    error!("udp server recv_from failed with error: {}", err);
                    continue;
                }
            };

            if self.context.check_outbound_blocked(&target_addr).await {
                error!("udp client {} outbound {} blocked by ACL rules", peer_addr, target_addr);
                continue;
            }

            let data = &buffer[..n];
            if let Err(err) = self.send_packet(&listener, peer_addr, target_addr, data).await {
                error!(
                    "udp packet relay {} with {} bytes failed, error: {}",
                    peer_addr,
                    data.len(),
                    err
                );
            }
        }
    }

    async fn send_packet(
        &mut self,
        listener: &Arc<MonProxySocket>,
        peer_addr: SocketAddr,
        target_addr: Address,
        data: &[u8],
    ) -> io::Result<()> {
        match self.assoc_map.lock().await.entry(peer_addr) {
            Entry::Occupied(occ) => {
                let assoc = occ.into_mut();
                assoc.try_send((target_addr, Bytes::copy_from_slice(data)))
            }
            Entry::Vacant(vac) => {
                let assoc = vac.insert(UdpAssociation::new(
                    self.context.clone(),
                    listener.clone(),
                    peer_addr,
                    self.assoc_map.clone(),
                ));
                trace!("created udp association for {}", peer_addr);
                assoc.try_send((target_addr, Bytes::copy_from_slice(data)))
            }
        }
    }
}

struct UdpAssociation {
    assoc: Arc<UdpAssociationContext>,
    sender: mpsc::Sender<(Address, Bytes)>,
}

impl Drop for UdpAssociation {
    fn drop(&mut self) {
        self.assoc.outbound_ipv4_socket.lock().abort();
        self.assoc.outbound_ipv6_socket.lock().abort();
    }
}

impl UdpAssociation {
    fn new(
        context: Arc<ServiceContext>,
        inbound: Arc<MonProxySocket>,
        peer_addr: SocketAddr,
        assoc_map: Arc<Mutex<LruCache<SocketAddr, UdpAssociation>>>,
    ) -> UdpAssociation {
        let (assoc, sender) = UdpAssociationContext::new(context, inbound, peer_addr, assoc_map);
        UdpAssociation { assoc, sender }
    }

    fn try_send(&self, data: (Address, Bytes)) -> io::Result<()> {
        if let Err(..) = self.sender.try_send(data) {
            let err = io::Error::new(ErrorKind::Other, "udp relay channel full");
            return Err(err);
        }
        Ok(())
    }
}

enum UdpAssociationState {
    Empty,
    Connected {
        socket: Arc<OutboundUdpSocket>,
        abortable: AbortHandle,
    },
    Aborted,
}

impl Drop for UdpAssociationState {
    fn drop(&mut self) {
        if let UdpAssociationState::Connected { ref abortable, .. } = *self {
            abortable.abort();
        }
    }
}

impl UdpAssociationState {
    fn empty() -> UdpAssociationState {
        UdpAssociationState::Empty
    }

    fn set_connected(&mut self, socket: Arc<OutboundUdpSocket>, abortable: AbortHandle) {
        *self = UdpAssociationState::Connected { socket, abortable };
    }

    fn abort(&mut self) {
        *self = UdpAssociationState::Aborted;
    }
}

struct UdpAssociationContext {
    context: Arc<ServiceContext>,
    inbound: Arc<MonProxySocket>,
    peer_addr: SocketAddr,
    outbound_ipv4_socket: SpinMutex<UdpAssociationState>,
    outbound_ipv6_socket: SpinMutex<UdpAssociationState>,
    assoc_map: Arc<Mutex<LruCache<SocketAddr, UdpAssociation>>>,
    target_cache: Mutex<LruCache<SocketAddr, Address>>,
}

impl Drop for UdpAssociationContext {
    fn drop(&mut self) {
        trace!("udp association for {} is closed", self.peer_addr);
    }
}

impl UdpAssociationContext {
    fn new(
        context: Arc<ServiceContext>,
        inbound: Arc<MonProxySocket>,
        peer_addr: SocketAddr,
        assoc_map: Arc<Mutex<LruCache<SocketAddr, UdpAssociation>>>,
    ) -> (Arc<UdpAssociationContext>, mpsc::Sender<(Address, Bytes)>) {
        // Pending packets 1024 should be good enough for a server.
        // If there are plenty of packets stuck in the channel, dropping exccess packets is a good way to protect the server from
        // being OOM.
        let (sender, receiver) = mpsc::channel(1024);

        let assoc = Arc::new(UdpAssociationContext {
            context,
            inbound,
            peer_addr,
            outbound_ipv4_socket: SpinMutex::new(UdpAssociationState::empty()),
            outbound_ipv6_socket: SpinMutex::new(UdpAssociationState::empty()),
            assoc_map,
            // Cache for remembering the original Address of target,
            // when recv_from a SocketAddr, we have to know whch Address that client was originally requested.
            //
            // XXX: 64 target addresses should be enough for __one__ client.
            target_cache: Mutex::new(LruCache::with_capacity(64)),
        });

        let l2r_task = {
            let assoc = assoc.clone();
            assoc.copy_l2r(receiver)
        };
        tokio::spawn(l2r_task);

        (assoc, sender)
    }

    async fn copy_l2r(self: Arc<Self>, mut receiver: mpsc::Receiver<(Address, Bytes)>) {
        while let Some((target_addr, data)) = receiver.recv().await {
            trace!(
                "udp relay {} -> {} with {} bytes",
                self.peer_addr,
                target_addr,
                data.len()
            );

            let assoc = self.clone();
            if let Err(err) = assoc.copy_l2r_dispatch(&target_addr, &data).await {
                error!(
                    "udp relay {} -> {} with {} bytes, error: {}",
                    self.peer_addr,
                    target_addr,
                    data.len(),
                    err
                );
            }
        }
    }

    async fn copy_l2r_dispatch(self: Arc<Self>, target_addr: &Address, data: &[u8]) -> io::Result<()> {
        match *target_addr {
            Address::SocketAddress(sa) => match sa {
                SocketAddr::V4(..) => self.copy_ipv4_l2r_dispatch(sa, data).await,
                SocketAddr::V6(..) => self.copy_ipv6_l2r_dispatch(sa, data).await,
            },
            Address::DomainNameAddress(ref dname, port) => {
                let sa = lookup_then!(self.context.context_ref(), dname, port, |sa| {
                    match sa {
                        SocketAddr::V4(..) => self.clone().copy_ipv4_l2r_dispatch(sa, data).await,
                        SocketAddr::V6(..) => self.clone().copy_ipv6_l2r_dispatch(sa, data).await,
                    }
                })?
                .0;

                // Record resolved address as reverse index
                self.target_cache.lock().await.insert(sa, target_addr.clone());

                Ok(())
            }
        }
    }

    async fn copy_ipv4_l2r_dispatch(self: Arc<Self>, target_addr: SocketAddr, data: &[u8]) -> io::Result<()> {
        let outbound = {
            let mut handle = self.outbound_ipv4_socket.lock();

            match *handle {
                UdpAssociationState::Empty => {
                    // Create a new connection to proxy server

                    let socket =
                        OutboundUdpSocket::connect_any_with_opts(&target_addr, self.context.connect_opts_ref()).await?;
                    let socket: Arc<OutboundUdpSocket> = Arc::new(socket.into());

                    let (r2l_fut, r2l_abortable) = {
                        let assoc = self.clone();
                        future::abortable(assoc.copy_r2l(socket.clone()))
                    };

                    // CLIENT <- REMOTE
                    tokio::spawn(r2l_fut);
                    debug!(
                        "created udp association for {} with {:?}",
                        self.peer_addr,
                        self.context.connect_opts_ref()
                    );

                    handle.set_connected(socket.clone(), r2l_abortable);
                    socket
                }
                UdpAssociationState::Connected { ref socket, .. } => socket.clone(),
                UdpAssociationState::Aborted => {
                    debug!(
                        "udp association for {} (bypassed) have been aborted, dropped packet {} bytes to {}",
                        self.peer_addr,
                        data.len(),
                        target_addr
                    );
                    return Ok(());
                }
            }
        };

        let n = outbound.send_to(data, target_addr).await?;
        if n != data.len() {
            warn!(
                "{} -> {} sent {} bytes != expected {} bytes",
                self.peer_addr,
                target_addr,
                n,
                data.len()
            );
        }

        Ok(())
    }

    async fn copy_ipv6_l2r_dispatch(self: Arc<Self>, target_addr: SocketAddr, data: &[u8]) -> io::Result<()> {
        let outbound = {
            let mut handle = self.outbound_ipv6_socket.lock();

            match *handle {
                UdpAssociationState::Empty => {
                    // Create a new connection to proxy server

                    let socket =
                        OutboundUdpSocket::connect_any_with_opts(&target_addr, self.context.connect_opts_ref()).await?;
                    let socket: Arc<OutboundUdpSocket> = Arc::new(socket.into());

                    let (r2l_fut, r2l_abortable) = {
                        let assoc = self.clone();
                        future::abortable(assoc.copy_r2l(socket.clone()))
                    };

                    // CLIENT <- REMOTE
                    tokio::spawn(r2l_fut);
                    debug!(
                        "created udp association for {} with {:?}",
                        self.peer_addr,
                        self.context.connect_opts_ref()
                    );

                    handle.set_connected(socket.clone(), r2l_abortable);
                    socket
                }
                UdpAssociationState::Connected { ref socket, .. } => socket.clone(),
                UdpAssociationState::Aborted => {
                    debug!(
                        "udp association for {} (bypassed) have been aborted, dropped packet {} bytes to {}",
                        self.peer_addr,
                        data.len(),
                        target_addr
                    );
                    return Ok(());
                }
            }
        };

        let n = outbound.send_to(data, target_addr).await?;
        if n != data.len() {
            warn!(
                "{} -> {} sent {} bytes != expected {} bytes",
                self.peer_addr,
                target_addr,
                n,
                data.len()
            );
        }

        Ok(())
    }

    async fn copy_r2l(self: Arc<Self>, outbound: Arc<OutboundUdpSocket>) -> io::Result<()> {
        let mut buffer = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        loop {
            let (n, addr) = match outbound.recv_from(&mut buffer).await {
                Ok(n) => {
                    // Keep association alive in map
                    let _ = self.assoc_map.lock().await.get(&self.peer_addr);
                    n
                }
                Err(err) => {
                    error!(
                        "udp failed to receive from outbound socket, peer_addr: {}, error: {}",
                        self.peer_addr, err
                    );
                    time::sleep(Duration::from_secs(1)).await;
                    continue;
                }
            };

            let data = &buffer[..n];

            let target_addr = match self.target_cache.lock().await.get(&addr) {
                Some(a) => a.clone(),
                None => Address::from(addr),
            };

            // Send back to client
            if let Err(err) = self.inbound.send_to(self.peer_addr, &target_addr, data).await {
                warn!(
                    "udp failed to send back to client {}, from target {} ({}), error: {}",
                    self.peer_addr, target_addr, addr, err
                );
            }

            trace!(
                "udp relay {} <- {} ({}) with {} bytes",
                self.peer_addr,
                target_addr,
                addr,
                data.len()
            );
        }
    }
}
