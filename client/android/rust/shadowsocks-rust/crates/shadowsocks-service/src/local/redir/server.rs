//! Shadowsocks Transparent Proxy Local Server

use std::{io, sync::Arc, time::Duration};

use futures::{future, FutureExt};
use shadowsocks::{config::Mode, ServerAddr};

use crate::{
    config::RedirType,
    local::{context::ServiceContext, loadbalancing::PingBalancer},
};

use super::{tcprelay::run_tcp_redir, udprelay::UdpRedir};

/// Transparent Proxy
pub struct Redir {
    context: Arc<ServiceContext>,
    mode: Mode,
    udp_expiry_duration: Option<Duration>,
    udp_capacity: Option<usize>,
    nodelay: bool,
    tcp_redir: RedirType,
    udp_redir: RedirType,
}

impl Redir {
    /// Create a new transparent proxy server with default configuration
    pub fn new() -> Redir {
        let context = ServiceContext::new();
        Redir::with_context(Arc::new(context))
    }

    /// Create a new transparent proxy server with context
    pub fn with_context(context: Arc<ServiceContext>) -> Redir {
        Redir {
            context,
            mode: Mode::TcpOnly,
            udp_expiry_duration: None,
            udp_capacity: None,
            nodelay: false,
            tcp_redir: RedirType::tcp_default(),
            udp_redir: RedirType::udp_default(),
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

    /// Set transparent proxy type of TCP relay, which is platform dependent
    pub fn set_tcp_redir(&mut self, ty: RedirType) {
        self.tcp_redir = ty;
    }

    /// Set transparent proxy type of UDP relay, which is platform dependent
    pub fn set_udp_redir(&mut self, ty: RedirType) {
        self.udp_redir = ty;
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
        run_tcp_redir(
            self.context.clone(),
            client_config,
            balancer,
            self.tcp_redir,
            self.nodelay,
        )
        .await
    }

    async fn run_udp_tunnel(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let server = UdpRedir::new(
            self.context.clone(),
            self.udp_redir,
            self.udp_expiry_duration,
            self.udp_capacity,
        );
        server.run(client_config, balancer).await
    }
}
