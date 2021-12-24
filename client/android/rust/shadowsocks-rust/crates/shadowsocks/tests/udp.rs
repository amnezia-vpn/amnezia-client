use std::{io, net::SocketAddr, sync::Arc};

use byte_string::ByteStr;
use log::info;
use tokio::{net::UdpSocket, sync::Barrier};

use shadowsocks::{
    config::{ServerConfig, ServerType},
    context::{Context, SharedContext},
    crypto::v1::CipherKind,
    relay::{socks5::Address, udprelay::ProxySocket},
};

async fn handle_udp_server_client(
    peer_addr: SocketAddr,
    remote_addr: Address,
    payload: &[u8],
    socket: &ProxySocket,
) -> io::Result<()> {
    let remote_socket = UdpSocket::bind("0.0.0.0:0").await?;

    match remote_addr {
        Address::SocketAddress(sa) => remote_socket.connect(sa).await?,
        Address::DomainNameAddress(ref domain, port) => remote_socket.connect((domain.as_str(), port)).await?,
    }

    remote_socket.send(payload).await?;

    let mut buf = [0u8; 65536];
    let n = remote_socket.recv(&mut buf).await?;

    socket.send_to(peer_addr, &remote_addr, &buf[..n]).await?;

    Ok(())
}

async fn handle_udp_local_client(
    context: SharedContext,
    svr_cfg: &ServerConfig,
    peer_addr: SocketAddr,
    remote_addr: Address,
    payload: &[u8],
    socket: &UdpSocket,
) -> io::Result<()> {
    let server_socket = ProxySocket::connect(context, svr_cfg).await?;
    server_socket.send(&remote_addr, payload).await?;

    let mut recv_buf = [0u8; 65536];
    let (n, ..) = server_socket.recv(&mut recv_buf).await?;
    socket.send_to(&recv_buf[..n], peer_addr).await?;

    Ok(())
}

async fn udp_tunnel_echo(
    server_addr: SocketAddr,
    local_addr: SocketAddr,
    target_addr: SocketAddr,
    password: &str,
    method: CipherKind,
) -> io::Result<()> {
    let svr_cfg_server = ServerConfig::new(server_addr, password, method);
    let svr_cfg_local = svr_cfg_server.clone();

    let ctx_server = Context::new_shared(ServerType::Server);
    let ctx_local = Context::new_shared(ServerType::Local);

    let barrier_server = Arc::new(Barrier::new(4));
    let barrier_local = barrier_server.clone();
    let barrier_target = barrier_server.clone();
    let barrier = barrier_server.clone();

    tokio::spawn(async move {
        let socket = UdpSocket::bind(target_addr).await.unwrap();
        barrier_target.wait().await;

        let mut buffer = vec![0u8; 65536];
        loop {
            let (n, peer_addr) = socket.recv_from(&mut buffer).await.unwrap();
            info!("echo packet: {:?}", ByteStr::new(&buffer[..n]));
            let _ = socket.send_to(&buffer[..n], peer_addr).await;
        }
    });

    tokio::spawn(async move {
        let svr_cfg_server = Arc::new(svr_cfg_server);
        let context = ctx_server;

        let socket = ProxySocket::bind(context.clone(), &svr_cfg_server).await.unwrap();
        barrier_server.wait().await;

        let mut recv_buf = vec![0u8; 65536];
        loop {
            let (n, peer_addr, remote_addr, ..) = socket.recv_from(&mut recv_buf).await.unwrap();
            let _ = handle_udp_server_client(peer_addr, remote_addr, &recv_buf[..n], &socket).await;
        }
    });

    tokio::spawn(async move {
        let svr_cfg_local = Arc::new(svr_cfg_local);

        let socket = UdpSocket::bind(local_addr).await.unwrap();
        barrier_local.wait().await;

        let context = ctx_local;

        let mut buffer = vec![0u8; 65536];
        loop {
            let (n, peer_addr) = socket.recv_from(&mut buffer).await.unwrap();
            let _ = handle_udp_local_client(
                context.clone(),
                &svr_cfg_local,
                peer_addr,
                target_addr.into(),
                &buffer[..n],
                &socket,
            )
            .await;
        }
    });

    barrier.wait().await;

    let socket = UdpSocket::bind("0.0.0.0:0").await?;
    socket.connect(local_addr).await?;

    static SEND_PAYLOAD: &[u8] = b"HELLO WORLD. \012345";
    socket.send(SEND_PAYLOAD).await?;

    let mut buffer = [0u8; 65536];
    let n = socket.recv(&mut buffer).await?;

    assert_eq!(&buffer[..n], SEND_PAYLOAD);

    Ok(())
}

#[tokio::test]
async fn udp_tunnel_aead() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:21001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:21101".parse::<SocketAddr>().unwrap();
    let target_addr = "127.0.0.1:21201".parse::<SocketAddr>().unwrap();

    udp_tunnel_echo(server_addr, local_addr, target_addr, "pas$$", CipherKind::AES_128_GCM)
        .await
        .unwrap();
}

#[cfg(feature = "stream-cipher")]
#[tokio::test]
async fn udp_tunnel_stream() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:22001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:22101".parse::<SocketAddr>().unwrap();
    let target_addr = "127.0.0.1:22201".parse::<SocketAddr>().unwrap();

    udp_tunnel_echo(
        server_addr,
        local_addr,
        target_addr,
        "pas$$",
        CipherKind::AES_128_CFB128,
    )
    .await
    .unwrap();
}

#[tokio::test]
async fn udp_tunnel_none() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:23001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:23101".parse::<SocketAddr>().unwrap();
    let target_addr = "127.0.0.1:23201".parse::<SocketAddr>().unwrap();

    udp_tunnel_echo(server_addr, local_addr, target_addr, "pas$$", CipherKind::NONE)
        .await
        .unwrap();
}
