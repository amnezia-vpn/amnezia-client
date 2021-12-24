#![cfg(all(feature = "local-socks4", feature = "server"))]

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
    local::socks::client::Socks4TcpClient,
    run_local,
    run_server,
    shadowsocks::{
        config::{ServerAddr, ServerConfig},
        crypto::v1::CipherKind,
    },
};

pub struct Socks4TestServer {
    local_addr: SocketAddr,
    svr_config: Config,
    cli_config: Config,
}

impl Socks4TestServer {
    pub fn new<S, L>(svr_addr: S, local_addr: L, pwd: &str, method: CipherKind) -> Socks4TestServer
    where
        S: ToSocketAddrs,
        L: ToSocketAddrs,
    {
        let svr_addr = svr_addr.to_socket_addrs().unwrap().next().unwrap();
        let local_addr = local_addr.to_socket_addrs().unwrap().next().unwrap();

        Socks4TestServer {
            local_addr,
            svr_config: {
                let mut cfg = Config::new(ConfigType::Server);
                cfg.server = vec![ServerConfig::new(svr_addr, pwd.to_owned(), method)];
                cfg
            },
            cli_config: {
                let mut cfg = Config::new(ConfigType::Local);
                cfg.local = vec![LocalConfig::new(ServerAddr::from(local_addr), ProtocolType::Socks)];
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

#[tokio::test]
async fn socks4_relay_connect() {
    let _ = env_logger::try_init();

    const SERVER_ADDR: &str = "127.0.0.1:7100";
    const LOCAL_ADDR: &str = "127.0.0.1:7200";

    const PASSWORD: &str = "test-password";
    const METHOD: CipherKind = CipherKind::AES_128_GCM;

    let svr = Socks4TestServer::new(SERVER_ADDR, LOCAL_ADDR, PASSWORD, METHOD);
    svr.run().await;

    static HTTP_REQUEST: &[u8] = b"GET / HTTP/1.0\r\nHost: www.example.com\r\nAccept: */*\r\n\r\n";

    let mut c = Socks4TcpClient::connect(("www.example.com", 80), LOCAL_ADDR, Vec::new())
        .await
        .unwrap();

    c.write_all(HTTP_REQUEST).await.unwrap();
    c.flush().await.unwrap();

    let mut buf = Vec::new();
    c.read_to_end(&mut buf).await.unwrap();

    println!("Got reply from server: {}", str::from_utf8(&buf).unwrap());

    let http_status = b"HTTP/1.0 200 OK\r\n";
    buf.starts_with(http_status);
}
