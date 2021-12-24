//! TCP Tunnel Server

use std::{io, net::SocketAddr, sync::Arc, time::Duration};

use log::{error, info, trace};
use shadowsocks::{lookup_then, net::TcpListener as ShadowTcpListener, relay::socks5::Address, ServerAddr};
use tokio::{net::TcpStream, time};

use crate::local::{
    context::ServiceContext,
    loadbalancing::PingBalancer,
    net::AutoProxyClientStream,
    utils::establish_tcp_tunnel,
};

pub async fn run_tcp_tunnel(
    context: Arc<ServiceContext>,
    client_config: &ServerAddr,
    balancer: PingBalancer,
    forward_addr: &Address,
    nodelay: bool,
) -> io::Result<()> {
    let listener = match *client_config {
        ServerAddr::SocketAddr(ref saddr) => ShadowTcpListener::bind_with_opts(saddr, context.accept_opts()).await?,
        ServerAddr::DomainName(ref dname, port) => {
            lookup_then!(context.context_ref(), dname, port, |addr| {
                ShadowTcpListener::bind_with_opts(&addr, context.accept_opts()).await
            })?
            .1
        }
    };

    info!("shadowsocks TCP tunnel listening on {}", listener.local_addr()?);

    loop {
        let (stream, peer_addr) = match listener.accept().await {
            Ok(s) => s,
            Err(err) => {
                error!("accept failed with error: {}", err);
                time::sleep(Duration::from_secs(1)).await;
                continue;
            }
        };

        if nodelay {
            let _ = stream.set_nodelay(true);
        }

        let balancer = balancer.clone();
        let forward_addr = forward_addr.clone();

        tokio::spawn(handle_tcp_client(
            context.clone(),
            stream,
            balancer,
            peer_addr,
            forward_addr,
            nodelay,
        ));
    }
}

async fn handle_tcp_client(
    context: Arc<ServiceContext>,
    mut stream: TcpStream,
    balancer: PingBalancer,
    peer_addr: SocketAddr,
    forward_addr: Address,
    nodelay: bool,
) -> io::Result<()> {
    let server = balancer.best_tcp_server();
    let svr_cfg = server.server_config();
    trace!(
        "establishing tcp tunnel {} <-> {} through sever {} (outbound: {})",
        peer_addr,
        forward_addr,
        svr_cfg.external_addr(),
        svr_cfg.addr(),
    );

    let remote = AutoProxyClientStream::connect_proxied(context, &server, &forward_addr).await?;

    if nodelay {
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
        &forward_addr,
    )
    .await
}
