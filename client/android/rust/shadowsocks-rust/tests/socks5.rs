#![cfg(all(feature = "local", feature = "server"))]

use std::{
    net::{SocketAddr, ToSocketAddrs},
    str,
};

use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    time::{self, Duration},
};

use shadowsocks_service::{
    config::{Config, ConfigType, LocalConfig, ProtocolType},
    local::socks::client::socks5::Socks5TcpClient,
    run_local,
    run_server,
    shadowsocks::{
        config::{Mode, ServerAddr, ServerConfig},
        crypto::v1::CipherKind,
        relay::socks5::Address,
    },
};

pub struct Socks5TestServer {
    local_addr: SocketAddr,
    svr_config: Config,
    cli_config: Config,
}

impl Socks5TestServer {
    pub fn new<S, L>(svr_addr: S, local_addr: L, pwd: &str, method: CipherKind, enable_udp: bool) -> Socks5TestServer
    where
        S: ToSocketAddrs,
        L: ToSocketAddrs,
    {
        let svr_addr = svr_addr.to_socket_addrs().unwrap().next().unwrap();
        let local_addr = local_addr.to_socket_addrs().unwrap().next().unwrap();

        Socks5TestServer {
            local_addr,
            svr_config: {
                let mut cfg = Config::new(ConfigType::Server);
                cfg.server = vec![ServerConfig::new(svr_addr, pwd.to_owned(), method)];
                cfg.server[0].set_mode(if enable_udp { Mode::TcpAndUdp } else { Mode::TcpOnly });
                cfg
            },
            cli_config: {
                let mut cfg = Config::new(ConfigType::Local);
                cfg.local = vec![LocalConfig::new(ServerAddr::from(local_addr), ProtocolType::Socks)];
                cfg.local[0].mode = if enable_udp { Mode::TcpAndUdp } else { Mode::TcpOnly };
                cfg.server = vec![ServerConfig::new(svr_addr, pwd.to_owned(), method)];
                cfg
            },
        }
    }

    pub fn client_addr(&self) -> &SocketAddr {
        &self.local_addr
    }

    pub async fn run(&self) {
        let svr_cfg = self.svr_config.clone();
        tokio::spawn(run_server(svr_cfg));

        let client_cfg = self.cli_config.clone();
        tokio::spawn(run_local(client_cfg));

        time::sleep(Duration::from_secs(1)).await;
    }
}

#[cfg(feature = "stream-cipher")]
#[tokio::test]
async fn socks5_relay_stream() {
    let _ = env_logger::try_init();

    const SERVER_ADDR: &str = "127.0.0.1:8100";
    const LOCAL_ADDR: &str = "127.0.0.1:8200";

    const PASSWORD: &str = "test-password";
    const METHOD: CipherKind = CipherKind::AES_128_CFB128;

    let svr = Socks5TestServer::new(SERVER_ADDR, LOCAL_ADDR, PASSWORD, METHOD, false);
    svr.run().await;

    let mut c = Socks5TcpClient::connect(
        Address::DomainNameAddress("www.example.com".to_owned(), 80),
        svr.client_addr(),
    )
    .await
    .unwrap();

    let req = b"GET / HTTP/1.0\r\nHost: www.example.com\r\nAccept: */*\r\n\r\n";
    c.write_all(req).await.unwrap();
    c.flush().await.unwrap();

    let mut buf = Vec::new();
    c.read_to_end(&mut buf).await.unwrap();

    println!("Got reply from server: {}", str::from_utf8(&buf).unwrap());

    let http_status = b"HTTP/1.0 200 OK\r\n";
    buf.starts_with(http_status);
}

#[tokio::test]
async fn socks5_relay_aead() {
    let _ = env_logger::try_init();

    const SERVER_ADDR: &str = "127.0.0.1:8110";
    const LOCAL_ADDR: &str = "127.0.0.1:8210";

    const PASSWORD: &str = "test-password";
    const METHOD: CipherKind = CipherKind::AES_256_GCM;

    let svr = Socks5TestServer::new(SERVER_ADDR, LOCAL_ADDR, PASSWORD, METHOD, false);
    svr.run().await;

    let mut c = Socks5TcpClient::connect(
        Address::DomainNameAddress("www.example.com".to_owned(), 80),
        svr.client_addr(),
    )
    .await
    .unwrap();

    let req = b"GET / HTTP/1.0\r\nHost: www.example.com\r\nAccept: */*\r\n\r\n";
    c.write_all(req).await.unwrap();
    c.flush().await.unwrap();

    let mut buf = Vec::new();
    c.read_to_end(&mut buf).await.unwrap();

    println!("Got reply from server: {}", str::from_utf8(&buf).unwrap());

    let http_status = b"HTTP/1.0 200 OK\r\n";
    buf.starts_with(http_status);
}
