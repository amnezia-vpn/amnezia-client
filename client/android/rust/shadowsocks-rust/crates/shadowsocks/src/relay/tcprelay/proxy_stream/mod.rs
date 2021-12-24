//! Stream interface for communicating with shadowsocks proxy servers

pub use self::{
    client::{ProxyClientStream, ProxyClientStreamReadHalf, ProxyClientStreamWriteHalf},
    server::{ProxyServerStream, ProxyServerStreamReadHalf, ProxyServerStreamWriteHalf},
};

pub mod client;
pub mod server;
