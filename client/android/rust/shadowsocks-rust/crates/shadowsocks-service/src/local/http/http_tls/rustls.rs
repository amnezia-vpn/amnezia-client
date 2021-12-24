//! TLS support by [rustls](https://crates.io/crates/rustls)

use std::{
    future::Future,
    io,
    net::SocketAddr,
    pin::Pin,
    task::{self, Poll},
};

use futures::ready;
use hyper::server::conn::AddrStream;
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

enum TlsStreamState {
    Handshaking(tokio_rustls::Accept<AddrStream>),
    Streaming(tokio_rustls::server::TlsStream<AddrStream>),
}

pub struct TlsStream {
    state: TlsStreamState,
    remote_addr: SocketAddr,
}

impl TlsStream {
    pub fn remote_addr(&self) -> SocketAddr {
        self.remote_addr
    }
}

macro_rules! forward_stream_method {
    ($self:expr, $cx:expr, $method:ident $(, $param:expr)*) => {{
        let this = $self.get_mut();

        loop {
            match this.state {
                TlsStreamState::Handshaking(ref mut accept_fut) => {
                    match ready!(Pin::new(accept_fut).poll($cx)) {
                        Ok(stream) => {
                            this.state = TlsStreamState::Streaming(stream);
                        }
                        Err(err) => {
                            let err = io::Error::new(io::ErrorKind::Other, format!("tls handshake: {}", err));
                            return Poll::Ready(Err(err));
                        }
                    }
                }
                TlsStreamState::Streaming(ref mut stream) => {
                    return Pin::new(stream).$method($cx, $($param),*);
                }
            }
        }
    }};
}

impl AsyncRead for TlsStream {
    fn poll_read(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        forward_stream_method!(self, cx, poll_read, buf)
    }
}

impl AsyncWrite for TlsStream {
    fn poll_write(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        forward_stream_method!(self, cx, poll_write, buf)
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        let this = self.get_mut();
        match this.state {
            TlsStreamState::Handshaking(..) => Poll::Ready(Ok(())),
            TlsStreamState::Streaming(ref mut stream) => Pin::new(stream).poll_flush(cx),
        }
    }

    fn poll_shutdown(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        let this = self.get_mut();
        match this.state {
            TlsStreamState::Handshaking(..) => Poll::Ready(Ok(())),
            TlsStreamState::Streaming(ref mut stream) => Pin::new(stream).poll_shutdown(cx),
        }
    }
}
