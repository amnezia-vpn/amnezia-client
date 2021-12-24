#![cfg(all(feature = "local-tunnel", feature = "server"))]

use std::str;

use byte_string::ByteStr;
use tokio::{
    self,
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpStream, UdpSocket},
    time::{self, Duration},
};

use shadowsocks_service::{
    config::{Config, ConfigType},
    run_local,
    run_server,
};

#[tokio::test]
async fn tcp_tunnel() {
    let _ = env_logger::try_init();

    let local_config = Config::load_from_str(
        r#"{
            "locals": [
                {
                    "local_port": 9110,
                    "local_address": "127.0.0.1",
                    "protocol": "tunnel",
                    "forward_address": "www.example.com",
                    "forward_port": 80
                }
            ],
            "server": "127.0.0.1",
            "server_port": 9120,
            "password": "password",
            "method": "aes-256-gcm"
        }"#,
        ConfigType::Local,
    )
    .unwrap();

    let server_config = Config::load_from_str(
        r#"{
            "server": "127.0.0.1",
            "server_port": 9120,
            "password": "password",
            "method": "aes-256-gcm"
        }"#,
        ConfigType::Server,
    )
    .unwrap();

    tokio::spawn(run_local(local_config));
    tokio::spawn(run_server(server_config));

    time::sleep(Duration::from_secs(1)).await;

    // Connect it directly, because it is now established a TCP tunnel with www.example.com
    let mut stream = TcpStream::connect("127.0.0.1:9110").await.unwrap();

    let req = b"GET / HTTP/1.0\r\nHost: www.example.com\r\nAccept: */*\r\n\r\n";
    stream.write_all(req).await.unwrap();
    stream.flush().await.unwrap();

    let mut buf = Vec::new();
    stream.read_to_end(&mut buf).await.unwrap();

    println!("Got reply from server: {}", str::from_utf8(&buf).unwrap());

    let http_status = b"HTTP/1.0 200 OK\r\n";
    buf.starts_with(http_status);
}

#[tokio::test]
async fn udp_tunnel() {
    // Query firefox.com, TransactionID: 0x1234
    static DNS_QUERY: &[u8] = b"\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07firefox\x03com\x00\x00\x01\x00\x01";

    let _ = env_logger::try_init();

    let local_config = Config::load_from_str(
        r#"{
            "locals": [
                {
                    "local_port": 9210,
                    "local_address": "127.0.0.1",
                    "protocol": "tunnel",
                    "forward_address": "8.8.8.8",
                    "forward_port": 53
                }
            ],
            "server": "127.0.0.1",
            "server_port": 9220,
            "password": "password",
            "method": "aes-256-gcm",
            "mode": "tcp_and_udp"
        }"#,
        ConfigType::Local,
    )
    .unwrap();

    let server_config = Config::load_from_str(
        r#"{
            "server": "127.0.0.1",
            "server_port": 9220,
            "password": "password",
            "method": "aes-256-gcm",
            "mode": "udp_only"
        }"#,
        ConfigType::Server,
    )
    .unwrap();

    tokio::spawn(run_local(local_config));
    tokio::spawn(run_server(server_config));

    time::sleep(Duration::from_secs(1)).await;

    let socket = UdpSocket::bind("0.0.0.0:0").await.unwrap();
    socket.send_to(DNS_QUERY, "127.0.0.1:9210").await.unwrap();

    let mut buf = vec![0u8; 65536];
    let n = socket.recv(&mut buf).await.unwrap();

    // DNS response have at least 12 bytes
    assert!(n >= 12);

    let recv_payload = &buf[..n];
    println!("Got reply from server: {:?}", ByteStr::new(&recv_payload));

    assert_eq!(b"\x12\x34", &recv_payload[0..2]);
}
