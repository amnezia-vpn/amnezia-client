//! TCP relay client implementation

use std::{
    io,
    pin::Pin,
    task::{self, Poll},
};

use log::trace;
use pin_project::pin_project;
use shadowsocks::relay::socks5::{
    self,
    Address,
    Command,
    Error,
    HandshakeRequest,
    HandshakeResponse,
    Reply,
    TcpRequestHeader,
    TcpResponseHeader,
};
use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    net::{TcpStream, ToSocketAddrs},
};

/// Socks5 proxy client
#[pin_project]
pub struct Socks5TcpClient {
    #[pin]
    stream: TcpStream,
}

impl Socks5TcpClient {
    /// Connects to `addr` via `proxy`
    pub async fn connect<A, P>(addr: A, proxy: P) -> Result<Socks5TcpClient, Error>
    where
        A: Into<Address>,
        P: ToSocketAddrs,
    {
        let mut s = TcpStream::connect(proxy).await?;

        // 1. Handshake
        let hs = HandshakeRequest::new(vec![socks5::SOCKS5_AUTH_METHOD_NONE]);
        trace!("client connected, going to send handshake: {:?}", hs);

        hs.write_to(&mut s).await?;

        let hsp = HandshakeResponse::read_from(&mut s).await?;

        trace!("got handshake response: {:?}", hsp);
        assert_eq!(hsp.chosen_method, socks5::SOCKS5_AUTH_METHOD_NONE);

        // 2. Send request header
        let h = TcpRequestHeader::new(Command::TcpConnect, addr.into());
        trace!("going to connect, req: {:?}", h);
        h.write_to(&mut s).await?;

        let hp = TcpResponseHeader::read_from(&mut s).await?;

        trace!("got response: {:?}", hp);
        match hp.reply {
            Reply::Succeeded => (),
            r => return Err(Error::Reply(r)),
        }

        Ok(Socks5TcpClient { stream: s })
    }

    /// UDP Associate `addr` via `proxy`
    ///
    /// According to RFC, `addr` is the address that your UDP socket binds to
    pub async fn udp_associate<A, P>(addr: A, proxy: P) -> Result<(Socks5TcpClient, Address), Error>
    where
        A: Into<Address>,
        P: ToSocketAddrs,
    {
        let mut s = TcpStream::connect(proxy).await?;

        // 1. Handshake
        let hs = HandshakeRequest::new(vec![socks5::SOCKS5_AUTH_METHOD_NONE]);
        trace!("client connected, going to send handshake: {:?}", hs);

        hs.write_to(&mut s).await?;

        let hsp = HandshakeResponse::read_from(&mut s).await?;

        trace!("got handshake response: {:?}", hsp);
        assert_eq!(hsp.chosen_method, socks5::SOCKS5_AUTH_METHOD_NONE);

        // 2. Send request header
        let h = TcpRequestHeader::new(Command::UdpAssociate, addr.into());
        trace!("going to connect, req: {:?}", h);

        h.write_to(&mut s).await?;
        let hp = TcpResponseHeader::read_from(&mut s).await?;

        trace!("got response: {:?}", hp);
        match hp.reply {
            Reply::Succeeded => (),
            r => return Err(Error::Reply(r)),
        }

        Ok((Socks5TcpClient { stream: s }, hp.address))
    }
}

impl AsyncRead for Socks5TcpClient {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut task::Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<Result<(), io::Error>> {
        self.project().stream.poll_read(cx, buf)
    }
}

impl AsyncWrite for Socks5TcpClient {
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
