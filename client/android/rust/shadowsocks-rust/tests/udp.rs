#![cfg_attr(clippy, allow(blacklisted_name))]
#![cfg(all(feature = "local", feature = "server"))]

use std::net::SocketAddr;

use log::debug;
use tokio::time::{self, Duration};

use shadowsocks_service::{
    config::{Config, ConfigType, LocalConfig, ProtocolType},
    local::socks::client::socks5::Socks5UdpClient,
    run_local,
    run_server,
    shadowsocks::{config::Mode, crypto::v1::CipherKind, relay::socks5::Address, ServerConfig},
};

const SERVER_ADDR: &str = "127.0.0.1:8093";
const LOCAL_ADDR: &str = "127.0.0.1:8291";

const UDP_ECHO_SERVER_ADDR: &str = "127.0.0.1:50403";

const PASSWORD: &str = "test-password";
const METHOD: CipherKind = CipherKind::AES_128_GCM;

fn get_svr_config() -> Config {
    let mut cfg = Config::new(ConfigType::Server);
    cfg.server = vec![ServerConfig::new(
        SERVER_ADDR.parse::<SocketAddr>().unwrap(),
        PASSWORD.to_owned(),
        METHOD,
    )];
    cfg.server[0].set_mode(Mode::TcpAndUdp);
    cfg
}

fn get_cli_config() -> Config {
    let mut cfg = Config::new(ConfigType::Local);
    cfg.local = vec![LocalConfig::new(LOCAL_ADDR.parse().unwrap(), ProtocolType::Socks)];
    cfg.local[0].mode = Mode::TcpAndUdp;
    cfg.server = vec![ServerConfig::new(
        SERVER_ADDR.parse::<SocketAddr>().unwrap(),
        PASSWORD.to_owned(),
        METHOD,
    )];
    cfg
}

fn get_client_addr() -> SocketAddr {
    LOCAL_ADDR.parse().unwrap()
}

fn start_server() {
    tokio::spawn(run_server(get_svr_config()));
}

fn start_local() {
    tokio::spawn(run_local(get_cli_config()));
}

fn start_udp_echo_server() {
    use tokio::net::UdpSocket;

    tokio::spawn(async {
        let l = UdpSocket::bind(UDP_ECHO_SERVER_ADDR).await.unwrap();

        debug!("UDP echo server started {}", UDP_ECHO_SERVER_ADDR);

        let mut buf = vec![0u8; 65536];
        let (amt, src) = l.recv_from(&mut buf).await.unwrap();

        debug!("UDP echo received {} bytes from {}", amt, src);

        l.send_to(&buf[..amt], &src).await.unwrap();

        debug!("UDP echo sent {} bytes to {}", amt, src);
    });
}

#[tokio::test]
async fn udp_relay() {
    let _ = env_logger::try_init();

    let remote_addr = Address::SocketAddress(UDP_ECHO_SERVER_ADDR.parse().unwrap());

    start_server();
    start_local();

    start_udp_echo_server();

    // Wait until all server starts
    time::sleep(Duration::from_secs(1)).await;

    let mut l = Socks5UdpClient::bind("127.0.0.1:0".parse::<SocketAddr>().unwrap())
        .await
        .unwrap();
    l.associate(&get_client_addr()).await.unwrap();

    let payload = b"HEllo WORld";
    l.send_to(0, payload, &remote_addr).await.unwrap();

    let mut buf = vec![0u8; 65536];
    let (amt, _, recv_addr) = time::timeout(Duration::from_secs(5), l.recv_from(&mut buf))
        .await
        .unwrap()
        .unwrap();
    println!("Received {} buf size={} {:?}", recv_addr, amt, &buf[..amt]);

    assert_eq!(recv_addr, remote_addr);
    assert_eq!(&buf[..amt], payload);
}
