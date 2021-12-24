//! Shadowsocks Core Library

#![crate_type = "lib"]

pub use self::{
    config::{ManagerAddr, ServerAddr, ServerConfig},
    manager::{ManagerClient, ManagerListener},
    relay::{
        tcprelay::{proxy_listener::ProxyListener, proxy_stream::ProxyClientStream},
        udprelay::proxy_socket::ProxySocket,
    },
};

pub use shadowsocks_crypto as crypto;

pub mod config;
pub mod context;
pub mod dns_resolver;
pub mod manager;
pub mod net;
pub mod plugin;
pub mod relay;
