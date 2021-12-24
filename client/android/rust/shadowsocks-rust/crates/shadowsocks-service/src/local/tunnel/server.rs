//! Shadowsocks Local Tunnel Server

use std::{io, sync::Arc, time::Duration};

use futures::{future, FutureExt};
use shadowsocks::{config::Mode, relay::socks5::Address, ServerAddr};

use crate::local::{context::ServiceContext, loadbalancing::PingBalancer};

use super::{tcprelay::run_tcp_tunnel, udprelay::UdpTunnel};

/// Tunnel Server
pub struct Tunnel {
    context: Arc<ServiceContext>,
    forward_addr: Address,
    mode: Mode,
    udp_expiry_duration: Option<Duration>,
    udp_capacity: Option<usize>,
    nodelay: bool,
}

impl Tunnel {
    /// Create a new Tunnel server forwarding to `forward_addr`
    pub fn new(forward_addr: Address) -> Tunnel {
        let context = ServiceContext::new();
        Tunnel::with_context(Arc::new(context), forward_addr)
    }

    /// Create a new Tunnel server with context
    pub fn with_context(context: Arc<ServiceContext>, forward_addr: Address) -> Tunnel {
        Tunnel {
            context,
            forward_addr,
            mode: Mode::TcpOnly,
            udp_expiry_duration: None,
            udp_capacity: None,
            nodelay: false,
        }
    }

    /// Set UDP association's expiry duration
    pub fn set_udp_expiry_duration(&mut self, d: Duration) {
        self.udp_expiry_duration = Some(d);
    }

    /// Set total UDP association to be kept simutaneously in server
    pub fn set_udp_capacity(&mut self, c: usize) {
        self.udp_capacity = Some(c);
    }

    /// Set server mode
    pub fn set_mode(&mut self, mode: Mode) {
        self.mode = mode;
    }

    /// Set `TCP_NODELAY`
    pub fn set_nodelay(&mut self, nodelay: bool) {
        self.nodelay = nodelay;
    }

    /// Start serving
    pub async fn run(self, tcp_addr: &ServerAddr, udp_addr: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let mut vfut = Vec::new();

        if self.mode.enable_tcp() {
            vfut.push(self.run_tcp_tunnel(tcp_addr, balancer.clone()).boxed());
        }

        if self.mode.enable_udp() {
            vfut.push(self.run_udp_tunnel(udp_addr, balancer).boxed());
        }

        let (res, ..) = future::select_all(vfut).await;
        res
    }

    async fn run_tcp_tunnel(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        run_tcp_tunnel(
            self.context.clone(),
            client_config,
            balancer,
            &self.forward_addr,
            self.nodelay,
        )
        .await
    }

    async fn run_udp_tunnel(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let mut server = UdpTunnel::new(self.context.clone(), self.udp_expiry_duration, self.udp_capacity);
        server.run(client_config, balancer, &self.forward_addr).await
    }
}
