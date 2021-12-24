use std::{
    io::{self},
    net::SocketAddr,
    sync::Arc,
};

use byte_string::ByteStr;
use futures::future;
use log::info;
use tokio::{
    io::{AsyncBufReadExt, AsyncWriteExt, BufReader},
    net::{TcpListener, TcpStream},
    sync::Barrier,
};

use shadowsocks::{
    config::{ServerConfig, ServerType},
    context::Context,
    crypto::v1::CipherKind,
    relay::{
        socks5::Address,
        tcprelay::{
            proxy_stream::ProxyServerStream,
            utils::{copy_from_encrypted, copy_to_encrypted},
        },
    },
    ProxyClientStream,
    ProxyListener,
};

async fn handle_tcp_tunnel_server_client(
    method: CipherKind,
    mut stream: ProxyServerStream<TcpStream>,
) -> io::Result<()> {
    let addr = Address::read_from(&mut stream).await?;

    let mut remote = {
        let remote = match addr {
            Address::SocketAddress(ref sa) => TcpStream::connect(sa).await?,
            Address::DomainNameAddress(ref dname, port) => TcpStream::connect((dname.as_str(), port)).await?,
        };

        info!("connected to remote {}", addr);
        remote
    };

    let (mut sr, mut sw) = stream.into_split();
    let (mut mr, mut mw) = remote.split();

    let l2r = copy_from_encrypted(method, &mut sr, &mut mw);
    let r2l = copy_to_encrypted(method, &mut mr, &mut sw);

    tokio::pin!(l2r);
    tokio::pin!(r2l);

    let _ = future::select(l2r, r2l).await;

    info!("TCP tunnel server finished");

    Ok(())
}

async fn handle_tcp_tunnel_local_client(
    context: Arc<Context>,
    svr_cfg: Arc<ServerConfig>,
    mut stream: TcpStream,
) -> io::Result<()> {
    let target_addr = Address::from(("www.example.com".to_owned(), 80));

    let remote = ProxyClientStream::connect(context, &svr_cfg, target_addr).await?;

    let (mut lr, mut lw) = stream.split();
    let (mut sr, mut sw) = remote.into_split();

    let l2s = copy_to_encrypted(svr_cfg.method(), &mut lr, &mut sw);
    let s2l = copy_from_encrypted(svr_cfg.method(), &mut sr, &mut lw);

    tokio::pin!(l2s);
    tokio::pin!(s2l);

    let _ = future::select(l2s, s2l).await;

    info!("TCP tunnel client finished");

    Ok(())
}

async fn tcp_tunnel_example(
    server_addr: SocketAddr,
    local_addr: SocketAddr,
    password: &str,
    method: CipherKind,
) -> io::Result<()> {
    let svr_cfg_server = ServerConfig::new(server_addr, password, method);
    let svr_cfg_local = svr_cfg_server.clone();

    let ctx_server = Context::new_shared(ServerType::Server);
    let ctx_local = Context::new_shared(ServerType::Local);

    let barrier_server = Arc::new(Barrier::new(3));
    let barrier_local = barrier_server.clone();
    let barrier = barrier_local.clone();

    tokio::spawn(async move {
        let svr_cfg_server = Arc::new(svr_cfg_server);

        let listener = ProxyListener::bind(ctx_server, &svr_cfg_server).await.unwrap();
        info!("server listening on {}", listener.local_addr().unwrap());

        barrier_server.wait().await;

        while let Ok((stream, peer_addr)) = listener.accept().await {
            info!("server accepted stream {}", peer_addr);
            tokio::spawn(handle_tcp_tunnel_server_client(svr_cfg_server.method(), stream));
        }
    });

    tokio::spawn(async move {
        let svr_cfg_local = Arc::new(svr_cfg_local);

        let listener = TcpListener::bind(local_addr).await.unwrap();
        info!("local listening on {}", listener.local_addr().unwrap());

        barrier_local.wait().await;

        while let Ok((stream, peer_addr)) = listener.accept().await {
            info!("local accepted stream {}", peer_addr);

            let context = ctx_local.clone();
            let svr_cfg = svr_cfg_local.clone();
            tokio::spawn(handle_tcp_tunnel_local_client(context, svr_cfg, stream));
        }
    });

    barrier.wait().await;

    let mut client = TcpStream::connect(local_addr).await?;

    static HTTP_REQUEST: &[u8] = b"GET / HTTP/1.0\r\nHost: www.example.com\r\nAccept: */*\r\nConnection: close\r\n\r\n";
    client.write_all(HTTP_REQUEST).await?;

    let mut reader = BufReader::new(client);

    let mut buffer = Vec::new();
    reader.read_until(b'\n', &mut buffer).await?;

    println!("{:?}", ByteStr::new(&buffer));

    static HTTP_RESPONSE_STATUS: &[u8] = b"HTTP/1.0 200 OK\r\n";
    assert!(buffer.starts_with(HTTP_RESPONSE_STATUS));

    Ok(())
}

#[tokio::test]
async fn tcp_tunnel_aead() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:31001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:31101".parse::<SocketAddr>().unwrap();
    tcp_tunnel_example(server_addr, local_addr, "p$p", CipherKind::AES_128_GCM)
        .await
        .unwrap();
}

#[cfg(feature = "stream-cipher")]
#[tokio::test]
async fn tcp_tunnel_stream() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:32001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:32101".parse::<SocketAddr>().unwrap();
    tcp_tunnel_example(server_addr, local_addr, "p$p", CipherKind::AES_128_CFB128)
        .await
        .unwrap();
}

#[tokio::test]
async fn tcp_tunnel_none() {
    let _ = env_logger::try_init();

    let server_addr = "127.0.0.1:33001".parse::<SocketAddr>().unwrap();
    let local_addr = "127.0.0.1:33101".parse::<SocketAddr>().unwrap();
    tcp_tunnel_example(server_addr, local_addr, "p$p", CipherKind::NONE)
        .await
        .unwrap();
}
