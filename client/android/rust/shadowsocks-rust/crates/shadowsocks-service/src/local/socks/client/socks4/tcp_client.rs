//! SOCKS 4/4a client implementation

use std::{
    io,
    pin::Pin,
    task::{self, Poll},
};

use log::trace;
use pin_project::pin_project;
use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    net::{TcpStream, ToSocketAddrs},
};

use crate::local::socks::socks4::{Address, Command, Error, HandshakeRequest, HandshakeResponse, ResultCode};

/// Socks4/4a proxy client
#[pin_project]
pub struct Socks4TcpClient {
    #[pin]
    stream: TcpStream,
}

impl Socks4TcpClient {
    /// Connects to `addr` via `proxy`
    pub async fn connect<A, P, U>(addr: A, proxy: P, user_id: U) -> Result<Socks4TcpClient, Error>
    where
        A: Into<Address>,
        P: ToSocketAddrs,
        U: Into<Vec<u8>>,
    {
        let mut s = TcpStream::connect(proxy).await?;

        // 1. handshake

        let hs = HandshakeRequest {
            cd: Command::Connect,
            dst: addr.into(),
            user_id: user_id.into(),
        };
        trace!("client connected, going to send handshake: {:?}", hs);

        hs.write_to(&mut s).await?;

        let hsp = HandshakeResponse::read_from(&mut s).await?;

        trace!("got handshake response: {:?}", hsp);

        if hsp.cd != ResultCode::RequestGranted {
            return Err(Error::Result(hsp.cd));
        }

        Ok(Socks4TcpClient { stream: s })
    }
}

impl AsyncRead for Socks4TcpClient {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut task::Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<Result<(), io::Error>> {
        self.project().stream.poll_read(cx, buf)
    }
}

impl AsyncWrite for Socks4TcpClient {
    fn poll_write(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<Result<usize, io::Error>> {
        self.project().stream.poll_write(cx, buf)
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.project().stream.poll_flush(cx)
    }

    fn poll_shutdown(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.project().stream.poll_shutdown(cx)
    }
}
