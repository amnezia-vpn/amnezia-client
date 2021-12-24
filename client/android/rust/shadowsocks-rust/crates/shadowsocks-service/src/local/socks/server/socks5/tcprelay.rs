//! SOCKS5 TCP Server

use std::{
    io::{self, ErrorKind},
    net::{Ipv4Addr, SocketAddr},
    sync::Arc,
};

use log::{debug, error, trace, warn};
use shadowsocks::{
    config::Mode,
    relay::socks5::{
        self,
        Address,
        Command,
        HandshakeRequest,
        HandshakeResponse,
        Reply,
        TcpRequestHeader,
        TcpResponseHeader,
    },
    ServerAddr,
};
use tokio::net::TcpStream;

use crate::{
    local::{
        context::ServiceContext,
        loadbalancing::PingBalancer,
        net::AutoProxyClientStream,
        utils::establish_tcp_tunnel,
    },
    net::utils::ignore_until_end,
};

pub struct Socks5TcpHandler {
    context: Arc<ServiceContext>,
    udp_bind_addr: Option<Arc<ServerAddr>>,
    nodelay: bool,
    balancer: PingBalancer,
    mode: Mode,
}

impl Socks5TcpHandler {
    pub fn new(
        context: Arc<ServiceContext>,
        udp_bind_addr: Option<Arc<ServerAddr>>,
        nodelay: bool,
        balancer: PingBalancer,
        mode: Mode,
    ) -> Socks5TcpHandler {
        Socks5TcpHandler {
            context,
            udp_bind_addr,
            nodelay,
            balancer,
            mode,
        }
    }

    pub async fn handle_socks5_client(self, mut stream: TcpStream, peer_addr: SocketAddr) -> io::Result<()> {
        // 1. Handshake

        let handshake_req = HandshakeRequest::read_from(&mut stream).await?;

        trace!("socks5 {:?}", handshake_req);

        if !handshake_req.methods.contains(&socks5::SOCKS5_AUTH_METHOD_NONE) {
            use std::io::Error;

            let resp = HandshakeResponse::new(socks5::SOCKS5_AUTH_METHOD_NOT_ACCEPTABLE);
            resp.write_to(&mut stream).await?;

            return Err(Error::new(
                ErrorKind::Other,
                "currently shadowsocks-rust does not support authentication",
            ));
        } else {
            // Reply to client
            let resp = HandshakeResponse::new(socks5::SOCKS5_AUTH_METHOD_NONE);
            trace!("reply handshake {:?}", resp);
            resp.write_to(&mut stream).await?;
        }

        // 2. Fetch headers
        let header = match TcpRequestHeader::read_from(&mut stream).await {
            Ok(h) => h,
            Err(err) => {
                error!("failed to get TcpRequestHeader: {}", err);
                let rh = TcpResponseHeader::new(err.as_reply(), Address::SocketAddress(peer_addr));
                rh.write_to(&mut stream).await?;
                return Err(err.into());
            }
        };

        trace!("socks5 {:?}", header);

        let addr = header.address;

        // 3. Handle Command
        match header.command {
            Command::TcpConnect => {
                debug!("CONNECT {}", addr);

                self.handle_tcp_connect(stream, peer_addr, addr).await
            }
            Command::UdpAssociate => {
                debug!("UDP ASSOCIATE from {}", addr);

                self.handle_udp_associate(stream, addr).await
            }
            Command::TcpBind => {
                warn!("BIND is not supported");
                let rh = TcpResponseHeader::new(socks5::Reply::CommandNotSupported, addr);
                rh.write_to(&mut stream).await?;

                Ok(())
            }
        }
    }

    async fn handle_tcp_connect(
        self,
        mut stream: TcpStream,
        peer_addr: SocketAddr,
        target_addr: Address,
    ) -> io::Result<()> {
        if !self.mode.enable_tcp() {
            warn!("TCP CONNECT is disabled");

            let rh = TcpResponseHeader::new(socks5::Reply::CommandNotSupported, target_addr);
            rh.write_to(&mut stream).await?;

            return Ok(());
        }

        let server = self.balancer.best_tcp_server();
        let svr_cfg = server.server_config();

        let remote = match AutoProxyClientStream::connect(self.context.clone(), &server, &target_addr).await {
            Ok(remote) => {
                // Tell the client that we are ready
                let header =
                    TcpResponseHeader::new(socks5::Reply::Succeeded, Address::SocketAddress(remote.local_addr()?));
                header.write_to(&mut stream).await?;

                trace!("sent header: {:?}", header);

                remote
            }
            Err(err) => {
                let reply = match err.kind() {
                    ErrorKind::ConnectionRefused => Reply::ConnectionRefused,
                    ErrorKind::ConnectionAborted => Reply::HostUnreachable,
                    _ => Reply::NetworkUnreachable,
                };

                let dummy_address = SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), 0);
                let header = TcpResponseHeader::new(reply, Address::SocketAddress(dummy_address));
                header.write_to(&mut stream).await?;

                return Err(err);
            }
        };

        if self.nodelay {
            remote.set_nodelay(true)?;
        }

        let (mut plain_reader, mut plain_writer) = stream.split();
        let (mut shadow_reader, mut shadow_writer) = remote.into_split();

        establish_tcp_tunnel(
            svr_cfg,
            &mut plain_reader,
            &mut plain_writer,
            &mut shadow_reader,
            &mut shadow_writer,
            peer_addr,
            &target_addr,
        )
        .await
    }

    async fn handle_udp_associate(self, mut stream: TcpStream, client_addr: Address) -> io::Result<()> {
        match self.udp_bind_addr {
            None => {
                warn!("socks5 udp is disabled");

                let rh = TcpResponseHeader::new(socks5::Reply::CommandNotSupported, client_addr);
                rh.write_to(&mut stream).await?;

                Ok(())
            }
            Some(bind_addr) => {
                // shadowsocks accepts both TCP and UDP from the same address

                let rh = TcpResponseHeader::new(socks5::Reply::Succeeded, bind_addr.as_ref().into());
                rh.write_to(&mut stream).await?;

                // Hold connection until EOF.
                let _ = ignore_until_end(&mut stream).await;

                Ok(())
            }
        }
    }
}
