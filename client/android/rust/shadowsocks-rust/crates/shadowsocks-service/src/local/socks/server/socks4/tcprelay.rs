//! Shadowsocks SOCKS4/4a Local Server

use std::{
    io::{self, ErrorKind},
    net::SocketAddr,
    sync::Arc,
};

use log::{debug, trace, warn};
use shadowsocks::config::Mode;
use tokio::{
    io::{AsyncWriteExt, BufReader},
    net::TcpStream,
};

use crate::local::{
    context::ServiceContext,
    loadbalancing::PingBalancer,
    net::AutoProxyClientStream,
    utils::establish_tcp_tunnel,
};

use crate::local::socks::socks4::{Address, Command, HandshakeRequest, HandshakeResponse, ResultCode};

pub struct Socks4TcpHandler {
    context: Arc<ServiceContext>,
    nodelay: bool,
    balancer: PingBalancer,
    mode: Mode,
}

impl Socks4TcpHandler {
    pub fn new(context: Arc<ServiceContext>, nodelay: bool, balancer: PingBalancer, mode: Mode) -> Socks4TcpHandler {
        Socks4TcpHandler {
            context,
            nodelay,
            balancer,
            mode,
        }
    }

    pub async fn handle_socks4_client(self, stream: TcpStream, peer_addr: SocketAddr) -> io::Result<()> {
        // 1. Handshake

        // NOTE: Wraps it with BufReader for reading NULL terminated informations in HandshakeRequest
        let mut s = BufReader::new(stream);
        let handshake_req = HandshakeRequest::read_from(&mut s).await?;

        trace!("socks4 {:?}", handshake_req);

        match handshake_req.cd {
            Command::Connect => {
                debug!("CONNECT {}", handshake_req.dst);

                self.handle_socks4_connect(s, peer_addr, handshake_req.dst).await
            }
            Command::Bind => {
                warn!("BIND is not supported");

                let handshake_rsp = HandshakeResponse::new(ResultCode::RequestRejectedOrFailed);
                handshake_rsp.write_to(&mut s).await?;

                Ok(())
            }
        }
    }

    async fn handle_socks4_connect(
        self,
        mut stream: BufReader<TcpStream>,
        peer_addr: SocketAddr,
        target_addr: Address,
    ) -> io::Result<()> {
        if !self.mode.enable_tcp() {
            warn!("TCP CONNECT is disabled");

            let handshake_rsp = HandshakeResponse::new(ResultCode::RequestRejectedOrFailed);
            handshake_rsp.write_to(&mut stream).await?;

            return Ok(());
        }

        let server = self.balancer.best_tcp_server();
        let svr_cfg = server.server_config();
        let target_addr = target_addr.into();

        let mut remote = match AutoProxyClientStream::connect(self.context, &server, &target_addr).await {
            Ok(remote) => {
                // Tell the client that we are ready
                let handshake_rsp = HandshakeResponse::new(ResultCode::RequestGranted);
                handshake_rsp.write_to(&mut stream).await?;

                trace!("sent header: {:?}", handshake_rsp);

                remote
            }
            Err(err) => {
                let result_code = match err.kind() {
                    ErrorKind::ConnectionRefused => ResultCode::RequestRejectedCannotConnect,
                    ErrorKind::ConnectionAborted => ResultCode::RequestRejectedCannotConnect,
                    _ => ResultCode::RequestRejectedOrFailed,
                };

                let handshake_rsp = HandshakeResponse::new(result_code);
                handshake_rsp.write_to(&mut stream).await?;

                return Err(err);
            }
        };

        if self.nodelay {
            remote.set_nodelay(true)?;
        }

        // NOTE: Transfer all buffered data before unwrap, or these data will be lost
        let buffer = stream.buffer();
        if !buffer.is_empty() {
            remote.write_all(buffer).await?;
        }

        // UNWRAP.
        let mut stream = stream.into_inner();

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
}
