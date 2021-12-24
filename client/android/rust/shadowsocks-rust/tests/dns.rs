#![cfg(all(feature = "local-dns", feature = "server"))]

use std::time::Duration;

use byteorder::{BigEndian, ByteOrder};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpStream, UdpSocket},
    time,
};

use shadowsocks_service::{
    config::{Config, ConfigType},
    run_local,
    run_server,
};

#[tokio::test]
async fn dns_relay() {
    let _ = env_logger::try_init();

    let local_config = Config::load_from_str(
        r#"{
            "locals": [
                {
                    "local_address": "127.0.0.1",
                    "local_port": 6110,
                    "protocol": "dns",
                    "local_dns_address": "114.114.114.114",
                    "remote_dns_address": "8.8.8.8"
                }
            ],
            "server": "127.0.0.1",
            "server_port": 6120,
            "password": "password",
            "method": "aes-256-gcm"
        }"#,
        ConfigType::Local,
    )
    .unwrap();

    let server_config = Config::load_from_str(
        r#"{
            "server": "127.0.0.1",
            "server_port": 6120,
            "password": "password",
            "method": "aes-256-gcm",
            "mode": "tcp_and_udp"
        }"#,
        ConfigType::Server,
    )
    .unwrap();

    tokio::spawn(run_local(local_config));
    tokio::spawn(run_server(server_config));

    time::sleep(Duration::from_secs(1)).await;

    // Query firefox.com, TransactionID: 0x1234
    static DNS_QUERY: &[u8] = b"\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07firefox\x03com\x00\x00\x01\x00\x01";

    // 1. DoT
    {
        let mut c = TcpStream::connect("127.0.0.1:6110").await.unwrap();

        let mut len_buf = [0u8; 2];
        BigEndian::write_u16(&mut len_buf, DNS_QUERY.len() as u16);
        c.write_all(&len_buf).await.unwrap();

        c.write_all(DNS_QUERY).await.unwrap();
        c.flush().await.unwrap();

        c.read_exact(&mut len_buf).await.unwrap();
        let resp_len = BigEndian::read_u16(&len_buf);

        let mut buf = vec![0u8; resp_len as usize];
        c.read_exact(&mut buf).await.unwrap();

        assert!(buf.starts_with(b"\x12\x34"));
    }

    // 2. DoU
    {
        let c = UdpSocket::bind("0.0.0.0:0").await.unwrap();
        c.send_to(DNS_QUERY, "127.0.0.1:6110").await.unwrap();

        let mut buf = [0u8; 65536];
        let n = c.recv(&mut buf).await.unwrap();
        assert!(n >= 12);

        let pkt = &buf[..n];
        assert_eq!(&pkt[..2], b"\x12\x34");
    }
}
