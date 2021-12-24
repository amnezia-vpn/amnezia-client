//! Load Balancer chooses server by statistic latency data collected from active probing

use std::{
    fmt::{self, Debug, Display},
    future::Future,
    io,
    net::{Ipv4Addr, SocketAddr},
    sync::{
        atomic::{AtomicUsize, Ordering},
        Arc,
    },
    time::{Duration, Instant},
};

use byte_string::ByteStr;
use futures::future::{self, AbortHandle};
use log::{debug, info, trace};
use shadowsocks::{
    config::Mode,
    relay::{
        socks5::Address,
        tcprelay::proxy_stream::ProxyClientStream,
        udprelay::{proxy_socket::ProxySocket, MAXIMUM_UDP_PAYLOAD_SIZE},
    },
    ServerConfig,
};
use tokio::{
    io::{AsyncBufReadExt, AsyncWriteExt, BufReader},
    time,
};

use crate::local::context::ServiceContext;

use super::{
    server_data::ServerIdent,
    server_stat::{Score, DEFAULT_CHECK_INTERVAL_SEC, DEFAULT_CHECK_TIMEOUT_SEC},
};

/// Remote Server Type
#[derive(Debug, Clone, Copy)]
pub enum ServerType {
    Tcp,
    Udp,
}

impl fmt::Display for ServerType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            ServerType::Tcp => f.write_str("TCP"),
            ServerType::Udp => f.write_str("UDP"),
        }
    }
}

/// Build a `PingBalancer`
pub struct PingBalancerBuilder {
    servers: Vec<Arc<ServerIdent>>,
    context: Arc<ServiceContext>,
    mode: Mode,
}

impl PingBalancerBuilder {
    pub fn new(context: Arc<ServiceContext>, mode: Mode) -> PingBalancerBuilder {
        PingBalancerBuilder {
            servers: Vec::new(),
            context,
            mode,
        }
    }

    pub fn add_server(&mut self, server: ServerIdent) {
        self.servers.push(Arc::new(server));
    }

    pub async fn build(self) -> (PingBalancer, impl Future<Output = ()>) {
        assert!(!self.servers.is_empty(), "build PingBalancer without any servers");

        let balancer_context = PingBalancerContext {
            servers: self.servers,
            best_tcp_idx: AtomicUsize::new(0),
            best_udp_idx: AtomicUsize::new(0),
            context: self.context,
            mode: self.mode,
        };

        balancer_context.init_score().await;

        let shared_context = Arc::new(balancer_context);

        let (checker, abortable) = {
            let shared_context = shared_context.clone();
            future::abortable(async move { shared_context.checker_task().await })
        };
        let checker = async move {
            let _ = checker.await;
        };

        let balancer = PingBalancer {
            inner: Arc::new(PingBalancerInner {
                context: shared_context,
                abortable,
            }),
        };
        (balancer, checker)
    }
}

struct PingBalancerContext {
    servers: Vec<Arc<ServerIdent>>,
    best_tcp_idx: AtomicUsize,
    best_udp_idx: AtomicUsize,
    context: Arc<ServiceContext>,
    mode: Mode,
}

impl PingBalancerContext {
    fn best_tcp_server(&self) -> Arc<ServerIdent> {
        self.servers[self.best_tcp_idx.load(Ordering::Relaxed)].clone()
    }

    fn best_udp_server(&self) -> Arc<ServerIdent> {
        self.servers[self.best_udp_idx.load(Ordering::Relaxed)].clone()
    }
}

impl PingBalancerContext {
    async fn init_score(&self) {
        assert!(!self.servers.is_empty(), "check PingBalancer without any servers");

        if self.servers.len() > 1 {
            self.check_once(true).await;
        }
    }

    async fn checker_task(self: Arc<Self>) {
        assert!(!self.servers.is_empty(), "check PingBalancer without any servers");

        if self.servers.len() == 1 {
            self.checker_task_dummy().await
        } else {
            self.checker_task_real().await
        }
    }

    /// Dummy task that will do nothing if there only have one server in the balancer
    async fn checker_task_dummy(self: Arc<Self>) {
        future::pending().await
    }

    /// Check each servers' score and update the best server's index
    async fn check_once(&self, first_run: bool) {
        let mut vfut = match self.mode {
            Mode::TcpAndUdp => Vec::with_capacity(self.servers.len() * 2),
            Mode::TcpOnly | Mode::UdpOnly => Vec::with_capacity(self.servers.len()),
        };

        for server in self.servers.iter() {
            if self.mode.enable_tcp() {
                let checker = PingChecker {
                    server: server.clone(),
                    server_type: ServerType::Tcp,
                    context: self.context.clone(),
                };
                vfut.push(checker.check_update_score());
            }

            if self.mode.enable_udp() {
                let checker = PingChecker {
                    server: server.clone(),
                    server_type: ServerType::Udp,
                    context: self.context.clone(),
                };
                vfut.push(checker.check_update_score());
            }
        }

        future::join_all(vfut).await;

        if self.mode.enable_tcp() {
            let old_best_idx = self.best_tcp_idx.load(Ordering::Acquire);

            let mut best_idx = 0;
            let mut best_score = u32::MAX;
            for (idx, server) in self.servers.iter().enumerate() {
                let score = server.tcp_score().score();
                if score < best_score {
                    best_idx = idx;
                    best_score = score;
                }
            }
            self.best_tcp_idx.store(best_idx, Ordering::Release);

            if first_run {
                info!(
                    "choosen best TCP server {}",
                    ServerConfigFormatter::new(self.servers[best_idx].server_config())
                );
            } else {
                if best_idx != old_best_idx {
                    info!(
                        "switched best TCP server from {} to {}",
                        ServerConfigFormatter::new(self.servers[old_best_idx].server_config()),
                        ServerConfigFormatter::new(self.servers[best_idx].server_config())
                    );
                } else {
                    debug!(
                        "kept best TCP server {}",
                        ServerConfigFormatter::new(self.servers[old_best_idx].server_config())
                    );
                }
            }
        }

        if self.mode.enable_udp() {
            let old_best_idx = self.best_udp_idx.load(Ordering::Acquire);

            let mut best_idx = 0;
            let mut best_score = u32::MAX;
            for (idx, server) in self.servers.iter().enumerate() {
                let score = server.udp_score().score();
                if score < best_score {
                    best_idx = idx;
                    best_score = score;
                }
            }
            self.best_udp_idx.store(best_idx, Ordering::Release);

            if first_run {
                info!(
                    "choosen best UDP server {}",
                    ServerConfigFormatter::new(self.servers[best_idx].server_config())
                );
            } else {
                if best_idx != old_best_idx {
                    info!(
                        "switched best UDP server from {} to {}",
                        ServerConfigFormatter::new(self.servers[old_best_idx].server_config()),
                        ServerConfigFormatter::new(self.servers[best_idx].server_config())
                    );
                } else {
                    debug!(
                        "kept best UDP server {}",
                        ServerConfigFormatter::new(self.servers[old_best_idx].server_config())
                    );
                }
            }
        }
    }

    async fn checker_task_real(&self) {
        loop {
            time::sleep(Duration::from_secs(DEFAULT_CHECK_INTERVAL_SEC)).await;

            // Sleep before check.
            // PingBalancer already checked once when constructing
            self.check_once(false).await;
        }
    }
}

struct PingBalancerInner {
    context: Arc<PingBalancerContext>,
    abortable: AbortHandle,
}

impl Drop for PingBalancerInner {
    fn drop(&mut self) {
        self.abortable.abort();
        trace!("ping balancer stopped");
    }
}

/// Balancer with active probing
#[derive(Clone)]
pub struct PingBalancer {
    inner: Arc<PingBalancerInner>,
}

impl PingBalancer {
    /// Get service context
    pub fn context(&self) -> Arc<ServiceContext> {
        self.inner.context.context.clone()
    }

    /// Get reference of the service context
    pub fn context_ref(&self) -> &ServiceContext {
        self.inner.context.context.as_ref()
    }

    /// Pick the best TCP server
    pub fn best_tcp_server(&self) -> Arc<ServerIdent> {
        self.inner.context.best_tcp_server()
    }

    /// Pick the best UDP server
    pub fn best_udp_server(&self) -> Arc<ServerIdent> {
        self.inner.context.best_udp_server()
    }
}

impl Debug for PingBalancer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("PingBalancer")
            .field("servers", &self.inner.context.servers)
            .field("best_tcp_idx", &self.inner.context.best_tcp_idx.load(Ordering::Relaxed))
            .field("best_udp_idx", &self.inner.context.best_udp_idx.load(Ordering::Relaxed))
            .finish()
    }
}

struct PingChecker {
    server: Arc<ServerIdent>,
    server_type: ServerType,
    context: Arc<ServiceContext>,
}

impl PingChecker {
    /// Checks server's score and update into `ServerScore<E>`
    async fn check_update_score(self) {
        let score = match self.check_delay().await {
            Ok(d) => match self.server_type {
                ServerType::Tcp => self.server.tcp_score().push_score(Score::Latency(d)).await,
                ServerType::Udp => self.server.udp_score().push_score(Score::Latency(d)).await,
            },
            // Penalty
            Err(..) => match self.server_type {
                ServerType::Tcp => self.server.tcp_score().push_score(Score::Errored).await,
                ServerType::Udp => self.server.udp_score().push_score(Score::Errored).await,
            },
        };

        trace!(
            "updated remote {} server {} (score: {})",
            self.server_type,
            self.server.server_config().addr(),
            score
        );
    }

    /// Detect TCP connectivity with Chromium [Network Portal Detection](https://www.chromium.org/chromium-os/chromiumos-design-docs/network-portal-detection)
    #[allow(dead_code)]
    async fn check_request_tcp_chromium(&self) -> io::Result<()> {
        static GET_BODY: &[u8] =
            b"GET /generate_204 HTTP/1.1\r\nHost: clients3.google.com\r\nConnection: close\r\nAccept: */*\r\n\r\n";

        let addr = Address::DomainNameAddress("clients3.google.com".to_owned(), 80);

        let mut stream = ProxyClientStream::connect_with_opts(
            self.context.context(),
            self.server.server_config(),
            &addr,
            self.context.connect_opts_ref(),
        )
        .await?;
        stream.write_all(GET_BODY).await?;

        let mut reader = BufReader::new(stream);

        let mut buf = Vec::new();
        reader.read_until(b'\n', &mut buf).await?;

        static EXPECTED_HTTP_STATUS_LINE: &[u8] = b"HTTP/1.1 204 No Content\r\n";
        if buf != EXPECTED_HTTP_STATUS_LINE {
            use std::io::{Error, ErrorKind};

            debug!(
                "unexpected response from http://clients3.google.com/generate_204, {:?}",
                ByteStr::new(&buf)
            );

            let err = Error::new(
                ErrorKind::InvalidData,
                "unexpected response from http://clients3.google.com/generate_204",
            );
            return Err(err);
        }

        Ok(())
    }

    /// Detect TCP connectivity with Firefox's http://detectportal.firefox.com/success.txt
    async fn check_request_tcp_firefox(&self) -> io::Result<()> {
        static GET_BODY: &[u8] =
            b"GET /success.txt HTTP/1.1\r\nHost: detectportal.firefox.com\r\nConnection: close\r\nAccept: */*\r\n\r\n";

        let addr = Address::DomainNameAddress("detectportal.firefox.com".to_owned(), 80);

        let mut stream = ProxyClientStream::connect_with_opts(
            self.context.context(),
            self.server.server_config(),
            &addr,
            self.context.connect_opts_ref(),
        )
        .await?;
        stream.write_all(GET_BODY).await?;

        let mut reader = BufReader::new(stream);

        let mut buf = Vec::new();
        reader.read_until(b'\n', &mut buf).await?;

        static EXPECTED_HTTP_STATUS_LINE: &[u8] = b"HTTP/1.1 200 OK\r\n";
        if buf != EXPECTED_HTTP_STATUS_LINE {
            use std::io::{Error, ErrorKind};

            debug!(
                "unexpected response from http://detectportal.firefox.com/success.txt, {:?}",
                ByteStr::new(&buf)
            );

            let err = Error::new(
                ErrorKind::InvalidData,
                "unexpected response from http://detectportal.firefox.com/success.txt",
            );
            return Err(err);
        }

        Ok(())
    }

    async fn check_request_udp(&self) -> io::Result<()> {
        // TransactionID: 0x1234
        // Flags: 0x0100 RD
        // Questions: 0x0001
        // Answer RRs: 0x0000
        // Authority RRs: 0x0000
        // Additional RRs: 0x0000
        // Queries
        //    - QNAME: \x07 firefox \x03 com \x00
        //    - QTYPE: 0x0001 A
        //    - QCLASS: 0x0001 IN
        static DNS_QUERY: &[u8] =
            b"\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07firefox\x03com\x00\x00\x01\x00\x01";

        let addr = Address::SocketAddress(SocketAddr::new(Ipv4Addr::new(8, 8, 8, 8).into(), 53));

        let client = ProxySocket::connect_with_opts(
            self.context.context(),
            self.server.server_config(),
            self.context.connect_opts_ref(),
        )
        .await?;
        client.send(&addr, DNS_QUERY).await?;

        let mut buffer = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        let (n, ..) = client.recv(&mut buffer).await?;

        let dns_answer = &buffer[..n];

        // DNS packet must have at least 6 * 2 bytes
        if dns_answer.len() < 12 || &dns_answer[0..2] != b"\x12\x34" {
            use std::io::{Error, ErrorKind};

            debug!("unexpected response from 8.8.8.8:53, {:?}", ByteStr::new(&dns_answer));

            let err = Error::new(ErrorKind::InvalidData, "unexpected response from 8.8.8.8:53");
            return Err(err);
        }

        Ok(())
    }

    async fn check_request(&self) -> io::Result<()> {
        match self.server_type {
            ServerType::Tcp => self.check_request_tcp_firefox().await,
            ServerType::Udp => self.check_request_udp().await,
        }
    }

    async fn check_delay(&self) -> io::Result<u32> {
        let start = Instant::now();

        // Send HTTP GET and read the first byte
        let timeout = Duration::from_secs(DEFAULT_CHECK_TIMEOUT_SEC);
        let res = time::timeout(timeout, self.check_request()).await;

        let elapsed = Instant::now() - start;
        let elapsed = elapsed.as_secs() as u32 * 1000 + elapsed.subsec_millis(); // Converted to ms
        match res {
            Ok(Ok(..)) => {
                // Got the result ... record its time
                trace!(
                    "checked remote {} server {} latency with {} ms",
                    self.server_type,
                    self.server.server_config().addr(),
                    elapsed
                );
                Ok(elapsed)
            }
            Ok(Err(err)) => {
                debug!(
                    "failed to check {} server {}, error: {}",
                    self.server_type,
                    self.server.server_config().addr(),
                    err
                );

                // NOTE: connection / handshake error, server is down
                Err(err)
            }
            Err(..) => {
                use std::io::ErrorKind;

                // Timeout
                trace!(
                    "checked remote {} server {} latency timeout, elapsed {} ms",
                    self.server_type,
                    self.server.server_config().addr(),
                    elapsed
                );

                // NOTE: timeout exceeded. Count as error.
                Err(ErrorKind::TimedOut.into())
            }
        }
    }
}

struct ServerConfigFormatter<'a> {
    server_config: &'a ServerConfig,
}

impl<'a> ServerConfigFormatter<'a> {
    fn new(server_config: &'a ServerConfig) -> ServerConfigFormatter<'a> {
        ServerConfigFormatter { server_config }
    }
}

impl Display for ServerConfigFormatter<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self.server_config.remarks() {
            None => Display::fmt(self.server_config.addr(), f),
            Some(remarks) => {
                if remarks.is_empty() {
                    Display::fmt(self.server_config.addr(), f)
                } else {
                    write!(f, "{} ({})", self.server_config.addr(), remarks)
                }
            }
        }
    }
}
