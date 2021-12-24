//! SIP002 URL Scheme
//!
//! SS-URI = "ss://" userinfo "@" hostname ":" port [ "/" ] [ "?" plugin ] [ "#" tag ]
//! userinfo = websafe-base64-encode-utf8(method  ":" password)

use clap::clap_app;
use qrcode::{types::Color, QrCode};

use shadowsocks_service::{
    config::{Config, ConfigType},
    shadowsocks::config::ServerConfig,
};

/// shadowsocks version
const VERSION: &str = env!("CARGO_PKG_VERSION");

const BLACK: &str = "\x1b[40m  \x1b[0m";
const WHITE: &str = "\x1b[47m  \x1b[0m";

fn print_qrcode(encoded: &str) {
    let qrcode = QrCode::new(encoded.as_bytes()).unwrap();

    for _ in 0..qrcode.width() + 2 {
        print!("{}", WHITE);
    }
    println!();

    for y in 0..qrcode.width() {
        print!("{}", WHITE);
        for x in 0..qrcode.width() {
            let color = match qrcode[(x, y)] {
                Color::Light => WHITE,
                Color::Dark => BLACK,
            };

            print!("{}", color);
        }
        println!("{}", WHITE);
    }

    for _ in 0..qrcode.width() + 2 {
        print!("{}", WHITE);
    }
    println!();
}

fn encode(filename: &str, need_qrcode: bool) {
    let config = Config::load_from_file(filename, ConfigType::Server).unwrap();

    for svr in config.server {
        let encoded = svr.to_url();

        println!("{}", encoded);

        if need_qrcode {
            let encoded = svr.to_qrcode_url();
            print_qrcode(&encoded);
        }
    }
}

fn decode(encoded: &str, need_qrcode: bool) {
    let svrconfig = ServerConfig::from_url(encoded).unwrap();

    let mut config = Config::new(ConfigType::Server);
    config.server.push(svrconfig);

    println!("{}", config);

    if need_qrcode {
        print_qrcode(encoded);
    }
}

fn main() {
    let app = clap_app!(ssurl =>
        (version: VERSION)
        (about: "Encode and decode ShadowSocks URL")
        (@arg ENCODE_CONFIG_PATH: -e --encode +takes_value conflicts_with[DECODE_CONFIG_PATH] required_unless[DECODE_CONFIG_PATH] "Encode the server configuration in the provided JSON file")
        (@arg DECODE_CONFIG_PATH: -d --decode +takes_value required_unless[ENCODE_CONFIG_PATH] "Decode the server configuration from the provide ShadowSocks URL")
        (@arg QRCODE: -c --qrcode !takes_value "Generate the QRCode with the provided configuration")
    );

    let matches = app.get_matches();

    let need_qrcode = matches.is_present("QRCODE");

    if let Some(file) = matches.value_of("ENCODE_CONFIG_PATH") {
        encode(file, need_qrcode);
    } else if let Some(encoded) = matches.value_of("DECODE_CONFIG_PATH") {
        decode(encoded, need_qrcode);
    } else {
        println!("Use -h for more detail");
    }
}
