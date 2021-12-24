//! Shadowsocks SOCKS Local Server

use std::{io, net::SocketAddr, sync::Arc, time::Duration};

use futures::{future, FutureExt};
use log::{error, info};
use shadowsocks::{config::Mode, lookup_then, net::TcpListener as ShadowTcpListener, ServerAddr};
use tokio::{net::TcpStream, time};

use crate::local::{context::ServiceContext, loadbalancing::PingBalancer};

#[cfg(feature = "local-socks4")]
use self::socks4::Socks4TcpHandler;
use self::socks5::{Socks5TcpHandler, Socks5UdpServer};

#[cfg(feature = "local-socks4")]
mod socks4;
mod socks5;

/// SOCKS4/4a, SOCKS5 Local Server
pub struct Socks {
    context: Arc<ServiceContext>,
    mode: Mode,
    udp_expiry_duration: Option<Duration>,
    udp_capacity: Option<usize>,
    udp_bind_addr: Option<ServerAddr>,
    nodelay: bool,
}

impl Socks {
    /// Create a new SOCKS server with default configuration
    pub fn new() -> Socks {
        let context = ServiceContext::new();
        Socks::with_context(Arc::new(context))
    }

    /// Create a new SOCKS server with context
    pub fn with_context(context: Arc<ServiceContext>) -> Socks {
        Socks {
            context,
            mode: Mode::TcpOnly,
            udp_expiry_duration: None,
            udp_capacity: None,
            udp_bind_addr: None,
            nodelay: false,
        }
    }

    /// Set server mode
    pub fn set_mode(&mut self, mode: Mode) {
        self.mode = mode;
    }

    /// Set UDP association's expiry duration
    pub fn set_udp_expiry_duration(&mut self, d: Duration) {
        self.udp_expiry_duration = Some(d);
    }

    /// Set total UDP association to be kept simutaneously in server
    pub fn set_udp_capacity(&mut self, c: usize) {
        self.udp_capacity = Some(c);
    }

    /// UDP server's bind address
    ///
    /// * If `mode` is `tcp_only`, then it will still return this address for `UDP_ASSOCIATE` command
    /// * Otherwise, UDP relay will bind to this address
    pub fn set_udp_bind_addr(&mut self, a: ServerAddr) {
        self.udp_bind_addr = Some(a);
    }

    /// Set `TCP_NODELAY`
    pub fn set_nodelay(&mut self, nodelay: bool) {
        self.nodelay = nodelay;
    }

    /// Start serving
    pub async fn run(self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let mut vfut = Vec::new();

        if self.mode.enable_tcp() {
            vfut.push(self.run_tcp_server(client_config, balancer.clone()).boxed());
        }

        if self.mode.enable_udp() {
            // NOTE: SOCKS 5 RFC requires TCP handshake for UDP ASSOCIATE command
            // But here we can start a standalone UDP SOCKS 5 relay server, for special use cases

            vfut.push(self.run_udp_server(client_config, balancer).boxed());
        }

        let (res, ..) = future::select_all(vfut).await;
        res
    }

    async fn run_tcp_server(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let listener = match *client_config {
            ServerAddr::SocketAddr(ref saddr) => {
                ShadowTcpListener::bind_with_opts(saddr, self.context.accept_opts()).await?
            }
            ServerAddr::DomainName(ref dname, port) => {
                lookup_then!(self.context.context_ref(), dname, port, |addr| {
                    ShadowTcpListener::bind_with_opts(&addr, self.context.accept_opts()).await
                })?
                .1
            }
        };

        info!("shadowsocks socks TCP listening on {}", listener.local_addr()?);

        // If UDP is enabled, SOCK5 UDP_ASSOCIATE command will let client to send requests to this address
        let udp_bind_addr = if self.mode.enable_udp() {
            let udp_bind_addr = self.udp_bind_addr.as_ref().unwrap_or(client_config);
            let udp_bind_addr = Arc::new(udp_bind_addr.clone());
            Some(udp_bind_addr)
        } else if let Some(ref ua) = self.udp_bind_addr {
            Some(Arc::new(ua.clone()))
        } else {
            None
        };

        loop {
            let (stream, peer_addr) = match listener.accept().await {
                Ok(s) => s,
                Err(err) => {
                    error!("accept failed with error: {}", err);
                    time::sleep(Duration::from_secs(1)).await;
                    continue;
                }
            };

            if self.nodelay {
                let _ = stream.set_nodelay(true);
            }

            let balancer = balancer.clone();
            let context = self.context.clone();
            let nodelay = self.nodelay;
            let udp_bind_addr = udp_bind_addr.clone();
            let mode = self.mode;

            tokio::spawn(Socks::handle_tcp_client(
                context,
                udp_bind_addr,
                stream,
                balancer,
                peer_addr,
                mode,
                nodelay,
            ));
        }
    }

    #[cfg(feature = "local-socks4")]
    async fn handle_tcp_client(
        context: Arc<ServiceContext>,
        udp_bind_addr: Option<Arc<ServerAddr>>,
        stream: TcpStream,
        balancer: PingBalancer,
        peer_addr: SocketAddr,
        mode: Mode,
        nodelay: bool,
    ) -> io::Result<()> {
        use std::io::ErrorKind;

        let mut version_buffer = [0u8; 1];
        let n = stream.peek(&mut version_buffer).await?;
        if n == 0 {
            return Err(ErrorKind::UnexpectedEof.into());
        }

        match version_buffer[0] {
            0x04 => {
                let handler = Socks4TcpHandler::new(context, nodelay, balancer, mode);
                handler.handle_socks4_client(stream, peer_addr).await
            }

            0x05 => {
                let handler = Socks5TcpHandler::new(context, udp_bind_addr, nodelay, balancer, mode);
                handler.handle_socks5_client(stream, peer_addr).await
            }

            version => {
                error!("unsupported socks version {:x}", version);
                let err = io::Error::new(ErrorKind::Other, "unsupported socks version");
                return Err(err);
            }
        }
    }

    #[cfg(not(feature = "local-socks4"))]
    async fn handle_tcp_client(
        context: Arc<ServiceContext>,
        udp_bind_addr: Option<Arc<ServerAddr>>,
        stream: TcpStream,
        balancer: PingBalancer,
        peer_addr: SocketAddr,
        mode: Mode,
        nodelay: bool,
    ) -> io::Result<()> {
        let handler = Socks5TcpHandler::new(context, udp_bind_addr, nodelay, balancer, mode);
        handler.handle_socks5_client(stream, peer_addr).await
    }

    async fn run_udp_server(&self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let server = Socks5UdpServer::new(self.context.clone(), self.udp_expiry_duration, self.udp_capacity);

        let udp_bind_addr = self.udp_bind_addr.as_ref().unwrap_or(client_config);
        server.run(udp_bind_addr, balancer).await
    }
}
