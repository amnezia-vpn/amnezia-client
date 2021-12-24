//! TcpStream wrappers that supports connecting with options

#[cfg(unix)]
use std::os::unix::io::{AsRawFd, FromRawFd, IntoRawFd};
#[cfg(windows)]
use std::os::windows::io::{AsRawSocket, FromRawSocket, IntoRawSocket};
use std::{
    io::{self, ErrorKind},
    net::SocketAddr,
    ops::{Deref, DerefMut},
    pin::Pin,
    task::{self, Poll},
};

use futures::{future, ready};
use log::{debug, warn};
use pin_project::pin_project;
use socket2::Socket;
use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    net::{TcpListener as TokioTcpListener, TcpSocket, TcpStream as TokioTcpStream},
};

use crate::{
    context::Context,
    relay::{socks5::Address, sys::tcp_stream_connect},
    ServerAddr,
};

use super::{AcceptOpts, ConnectOpts};

/// TcpStream for outbound connections
#[pin_project]
pub struct TcpStream(#[pin] TokioTcpStream);

impl TcpStream {
    /// Connects to address
    pub async fn connect_with_opts(addr: &SocketAddr, opts: &ConnectOpts) -> io::Result<TcpStream> {
        tcp_stream_connect(addr, opts).await.map(TcpStream)
    }

    /// Connects shadowsocks server
    pub async fn connect_server_with_opts(
        context: &Context,
        addr: &ServerAddr,
        opts: &ConnectOpts,
    ) -> io::Result<TcpStream> {
        let stream = match *addr {
            ServerAddr::SocketAddr(ref addr) => tcp_stream_connect(addr, opts).await?,
            ServerAddr::DomainName(ref domain, port) => {
                lookup_then!(&context, &domain, port, |addr| {
                    tcp_stream_connect(&addr, opts).await
                })?
                .1
            }
        };

        Ok(TcpStream(stream))
    }

    /// Connects proxy remote target
    pub async fn connect_remote_with_opts(
        context: &Context,
        addr: &Address,
        opts: &ConnectOpts,
    ) -> io::Result<TcpStream> {
        let stream = match *addr {
            Address::SocketAddress(ref addr) => tcp_stream_connect(addr, opts).await?,
            Address::DomainNameAddress(ref domain, port) => {
                lookup_then!(&context, &domain, port, |addr| {
                    tcp_stream_connect(&addr, opts).await
                })?
                .1
            }
        };

        Ok(TcpStream(stream))
    }
}

impl Deref for TcpStream {
    type Target = TokioTcpStream;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for TcpStream {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl AsyncRead for TcpStream {
    fn poll_read(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        self.project().0.poll_read(cx, buf)
    }
}

impl AsyncWrite for TcpStream {
    fn poll_write(self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        self.project().0.poll_write(cx, buf)
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        self.project().0.poll_flush(cx)
    }

    fn poll_shutdown(self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        self.project().0.poll_shutdown(cx)
    }
}

impl From<TokioTcpStream> for TcpStream {
    fn from(s: TokioTcpStream) -> TcpStream {
        TcpStream(s)
    }
}

impl Into<TokioTcpStream> for TcpStream {
    fn into(self) -> TokioTcpStream {
        self.0
    }
}

/// `TcpListener` for accepting inbound connections
pub struct TcpListener {
    inner: TokioTcpListener,
    accept_opts: AcceptOpts,
}

impl TcpListener {
    /// Creates a new TcpListener, which will be bound to the specified address.
    pub async fn bind_with_opts(addr: &SocketAddr, accept_opts: AcceptOpts) -> io::Result<TcpListener> {
        let set_dual_stack = if let SocketAddr::V6(ref v6) = *addr {
            v6.ip().is_unspecified()
        } else {
            false
        };

        if !set_dual_stack {
            let inner = TokioTcpListener::bind(addr).await?;
            Ok(TcpListener { inner, accept_opts })
        } else {
            let socket = match *addr {
                SocketAddr::V4(..) => TcpSocket::new_v4()?,
                SocketAddr::V6(..) => TcpSocket::new_v6()?,
            };

            // https://docs.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse
            #[cfg(not(windows))]
            socket.set_reuseaddr(true)?;

            // Set to DUAL STACK mode by default.
            // WARNING: This would fail if you want to start another program listening on the same port.
            //
            // Should this behavior be configurable?
            fn set_only_v6(socket: &TcpSocket, only_v6: bool) {
                unsafe {
                    // WARN: If the following code panics, FD will be closed twice.
                    #[cfg(unix)]
                    let s = Socket::from_raw_fd(socket.as_raw_fd());
                    #[cfg(windows)]
                    let s = Socket::from_raw_socket(socket.as_raw_socket());
                    if let Err(err) = s.set_only_v6(only_v6) {
                        warn!("failed to set IPV6_V6ONLY: {} for listener, error: {}", only_v6, err);

                        // This is not a fatal error, just warn and skip
                    }

                    #[cfg(unix)]
                    let _ = s.into_raw_fd();
                    #[cfg(windows)]
                    let _ = s.into_raw_socket();
                }
            }

            set_only_v6(&socket, false);
            match socket.bind(*addr) {
                Ok(..) => {}
                Err(ref err) if err.kind() == ErrorKind::AddrInUse => {
                    // This is probably 0.0.0.0 with the same port has already been occupied
                    debug!(
                        "0.0.0.0:{} may have already been occupied, retry with IPV6_V6ONLY",
                        addr.port()
                    );

                    set_only_v6(&socket, true);
                    socket.bind(*addr)?;
                }
                Err(err) => return Err(err),
            }

            // mio's default backlog is 1024
            let inner = socket.listen(1024)?;
            Ok(TcpListener { inner, accept_opts })
        }
    }

    /// Create a `TcpListener` from tokio's `TcpListener`
    pub fn from_listener(listener: TokioTcpListener, accept_opts: AcceptOpts) -> TcpListener {
        TcpListener {
            inner: listener,
            accept_opts,
        }
    }

    /// Polls to accept a new incoming connection to this listener.
    pub fn poll_accept(&self, cx: &mut task::Context<'_>) -> Poll<io::Result<(TokioTcpStream, SocketAddr)>> {
        let (stream, peer_addr) = ready!(self.inner.poll_accept(cx))?;
        setsockopt_with_opt(&stream, &self.accept_opts)?;
        Poll::Ready(Ok((stream, peer_addr)))
    }

    /// Accept a new incoming connection to this listener
    pub async fn accept(&self) -> io::Result<(TokioTcpStream, SocketAddr)> {
        future::poll_fn(|cx| self.poll_accept(cx)).await
    }
}

impl Deref for TcpListener {
    type Target = TokioTcpListener;

    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl DerefMut for TcpListener {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.inner
    }
}

impl Into<TokioTcpListener> for TcpListener {
    fn into(self) -> TokioTcpListener {
        self.inner
    }
}

#[cfg(unix)]
fn setsockopt_with_opt<F: AsRawFd>(f: &F, opts: &AcceptOpts) -> io::Result<()> {
    let socket = unsafe { Socket::from_raw_fd(f.as_raw_fd()) };

    if let Some(buf_size) = opts.tcp.send_buffer_size {
        socket.set_send_buffer_size(buf_size as usize)?;
    }

    if let Some(buf_size) = opts.tcp.recv_buffer_size {
        socket.set_recv_buffer_size(buf_size as usize)?;
    }

    if opts.tcp.nodelay {
        socket.set_nodelay(true)?;
    }

    let _ = socket.into_raw_fd();
    Ok(())
}

#[cfg(windows)]
fn setsockopt_with_opt<F: AsRawSocket>(f: &F, opts: &AcceptOpts) -> io::Result<()> {
    let socket = unsafe { Socket::from_raw_socket(f.as_raw_socket()) };

    if let Some(buf_size) = opts.tcp.send_buffer_size {
        socket.set_send_buffer_size(buf_size as usize)?;
    }

    if let Some(buf_size) = opts.tcp.recv_buffer_size {
        socket.set_recv_buffer_size(buf_size as usize)?;
    }

    if opts.tcp.nodelay {
        socket.set_nodelay(true)?;
    }

    let _ = socket.into_raw_socket();
    Ok(())
}
