//! A `ProxyStream` that bypasses or proxies data through proxy server automatically

use std::{
    io::{self, IoSlice},
    net::SocketAddr,
    pin::Pin,
    sync::Arc,
    task::{self, Poll},
};

use pin_project::pin_project;
use shadowsocks::{
    net::TcpStream,
    relay::{
        socks5::Address,
        tcprelay::proxy_stream::{ProxyClientStream, ProxyClientStreamReadHalf, ProxyClientStreamWriteHalf},
    },
};
use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    net::{
        tcp::{OwnedReadHalf, OwnedWriteHalf},
        TcpStream as TokioTcpStream,
    },
};

use crate::{
    local::{context::ServiceContext, loadbalancing::ServerIdent},
    net::MonProxyStream,
};

use super::auto_proxy_io::AutoProxyIo;

/// Unified stream for bypassed and proxied connections
#[pin_project(project = AutoProxyClientStreamProj)]
pub enum AutoProxyClientStream {
    Proxied(#[pin] ProxyClientStream<MonProxyStream<TokioTcpStream>>),
    Bypassed(#[pin] TokioTcpStream),
}

impl AutoProxyClientStream {
    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`
    pub async fn connect<A>(
        context: Arc<ServiceContext>,
        server: &ServerIdent,
        addr: A,
    ) -> io::Result<AutoProxyClientStream>
    where
        A: Into<Address>,
    {
        let addr = addr.into();
        if context.check_target_bypassed(&addr).await {
            AutoProxyClientStream::connect_bypassed(context, addr).await
        } else {
            AutoProxyClientStream::connect_proxied(context, server, addr).await
        }
    }

    /// Connect directly to target `addr`
    pub async fn connect_bypassed<A>(context: Arc<ServiceContext>, addr: A) -> io::Result<AutoProxyClientStream>
    where
        A: Into<Address>,
    {
        // Connect directly.
        let addr = addr.into();
        let stream =
            TcpStream::connect_remote_with_opts(context.context_ref(), &addr, context.connect_opts_ref()).await?;
        Ok(AutoProxyClientStream::Bypassed(stream.into()))
    }

    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`
    pub async fn connect_proxied<A>(
        context: Arc<ServiceContext>,
        server: &ServerIdent,
        addr: A,
    ) -> io::Result<AutoProxyClientStream>
    where
        A: Into<Address>,
    {
        let flow_stat = context.flow_stat();
        let stream = match ProxyClientStream::connect_with_opts_map(
            context.context(),
            server.server_config(),
            addr,
            context.connect_opts_ref(),
            |stream| MonProxyStream::from_stream(stream, flow_stat),
        )
        .await
        {
            Ok(s) => s,
            Err(err) => {
                server.tcp_score().report_failure().await;
                return Err(err);
            }
        };
        Ok(AutoProxyClientStream::Proxied(stream))
    }

    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        match *self {
            AutoProxyClientStream::Proxied(ref s) => s.get_ref().get_ref().local_addr(),
            AutoProxyClientStream::Bypassed(ref s) => s.local_addr(),
        }
    }

    pub fn set_nodelay(&self, nodelay: bool) -> io::Result<()> {
        match *self {
            AutoProxyClientStream::Proxied(ref s) => s.get_ref().get_ref().set_nodelay(nodelay),
            AutoProxyClientStream::Bypassed(ref s) => s.set_nodelay(nodelay),
        }
    }
}

impl AutoProxyIo for AutoProxyClientStream {
    fn is_proxied(&self) -> bool {
        matches!(*self, AutoProxyClientStream::Proxied(..))
    }
}

impl AsyncRead for AutoProxyClientStream {
    fn poll_read(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamProj::Proxied(s) => s.poll_read(cx, buf),
            AutoProxyClientStreamProj::Bypassed(s) => s.poll_read(cx, buf),
        }
    }
}

impl AsyncWrite for AutoProxyClientStream {
    fn poll_write(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        match self.project() {
            AutoProxyClientStreamProj::Proxied(s) => s.poll_write(cx, buf),
            AutoProxyClientStreamProj::Bypassed(s) => s.poll_write(cx, buf),
        }
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamProj::Proxied(s) => s.poll_flush(cx),
            AutoProxyClientStreamProj::Bypassed(s) => s.poll_flush(cx),
        }
    }

    fn poll_shutdown(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamProj::Proxied(s) => s.poll_shutdown(cx),
            AutoProxyClientStreamProj::Bypassed(s) => s.poll_shutdown(cx),
        }
    }

    fn poll_write_vectored(
        self: Pin<&mut Self>,
        cx: &mut task::Context<'_>,
        bufs: &[IoSlice<'_>],
    ) -> Poll<io::Result<usize>> {
        match self.project() {
            AutoProxyClientStreamProj::Proxied(s) => s.poll_write_vectored(cx, bufs),
            AutoProxyClientStreamProj::Bypassed(s) => s.poll_write_vectored(cx, bufs),
        }
    }
}

impl From<ProxyClientStream<MonProxyStream<TokioTcpStream>>> for AutoProxyClientStream {
    fn from(s: ProxyClientStream<MonProxyStream<TokioTcpStream>>) -> Self {
        AutoProxyClientStream::Proxied(s)
    }
}

impl AutoProxyClientStream {
    pub fn into_split(self) -> (AutoProxyClientStreamReadHalf, AutoProxyClientStreamWriteHalf) {
        match self {
            AutoProxyClientStream::Proxied(s) => {
                let (r, w) = s.into_split();
                (
                    AutoProxyClientStreamReadHalf::Proxied(r),
                    AutoProxyClientStreamWriteHalf::Proxied(w),
                )
            }
            AutoProxyClientStream::Bypassed(s) => {
                let (r, w) = s.into_split();
                (
                    AutoProxyClientStreamReadHalf::Bypassed(r),
                    AutoProxyClientStreamWriteHalf::Bypassed(w),
                )
            }
        }
    }
}

#[pin_project(project = AutoProxyClientStreamReadHalfProj)]
pub enum AutoProxyClientStreamReadHalf {
    Proxied(#[pin] ProxyClientStreamReadHalf<MonProxyStream<TokioTcpStream>>),
    Bypassed(#[pin] OwnedReadHalf),
}

impl AutoProxyIo for AutoProxyClientStreamReadHalf {
    fn is_proxied(&self) -> bool {
        matches!(*self, AutoProxyClientStreamReadHalf::Proxied(..))
    }
}

impl AsyncRead for AutoProxyClientStreamReadHalf {
    fn poll_read(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamReadHalfProj::Proxied(s) => s.poll_read(cx, buf),
            AutoProxyClientStreamReadHalfProj::Bypassed(s) => s.poll_read(cx, buf),
        }
    }
}

#[pin_project(project = AutoProxyClientStreamWriteHalfProj)]
pub enum AutoProxyClientStreamWriteHalf {
    Proxied(#[pin] ProxyClientStreamWriteHalf<MonProxyStream<TokioTcpStream>>),
    Bypassed(#[pin] OwnedWriteHalf),
}

impl AutoProxyIo for AutoProxyClientStreamWriteHalf {
    fn is_proxied(&self) -> bool {
        matches!(*self, AutoProxyClientStreamWriteHalf::Proxied(..))
    }
}

impl AsyncWrite for AutoProxyClientStreamWriteHalf {
    fn poll_write(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        match self.project() {
            AutoProxyClientStreamWriteHalfProj::Proxied(s) => s.poll_write(cx, buf),
            AutoProxyClientStreamWriteHalfProj::Bypassed(s) => s.poll_write(cx, buf),
        }
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamWriteHalfProj::Proxied(s) => s.poll_flush(cx),
            AutoProxyClientStreamWriteHalfProj::Bypassed(s) => s.poll_flush(cx),
        }
    }

    fn poll_shutdown(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        match self.project() {
            AutoProxyClientStreamWriteHalfProj::Proxied(s) => s.poll_shutdown(cx),
            AutoProxyClientStreamWriteHalfProj::Bypassed(s) => s.poll_shutdown(cx),
        }
    }

    fn poll_write_vectored(
        self: Pin<&mut Self>,
        cx: &mut task::Context<'_>,
        bufs: &[IoSlice<'_>],
    ) -> Poll<io::Result<usize>> {
        match self.project() {
            AutoProxyClientStreamWriteHalfProj::Proxied(s) => s.poll_write_vectored(cx, bufs),
            AutoProxyClientStreamWriteHalfProj::Bypassed(s) => s.poll_write_vectored(cx, bufs),
        }
    }
}
