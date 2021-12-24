//! TCP stream for communicating with shadowsocks' proxy server

use std::{
    io::{self, ErrorKind},
    pin::Pin,
    task::{self, Poll},
};

use bytes::{BufMut, BytesMut};
use futures::ready;
use log::trace;
use once_cell::sync::Lazy;
use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    net::TcpStream,
    time,
};

use crate::{
    config::ServerConfig,
    context::SharedContext,
    net::{ConnectOpts, TcpStream as OutboundTcpStream},
    relay::{
        socks5::Address,
        tcprelay::crypto_io::{CryptoStream, CryptoStreamReadHalf, CryptoStreamWriteHalf},
    },
};

/// A stream for sending / receiving data stream from remote server via shadowsocks' proxy server
pub struct ProxyClientStream<S> {
    stream: CryptoStream<S>,
    addr: Option<Address>,
    context: SharedContext,
}

static DEFAULT_CONNECT_OPTS: Lazy<ConnectOpts> = Lazy::new(Default::default);

impl ProxyClientStream<TcpStream> {
    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`
    pub async fn connect<A>(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        addr: A,
    ) -> io::Result<ProxyClientStream<TcpStream>>
    where
        A: Into<Address>,
    {
        ProxyClientStream::connect_with_opts(context, svr_cfg, addr, &DEFAULT_CONNECT_OPTS).await
    }

    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`
    pub async fn connect_with_opts<A>(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        addr: A,
        opts: &ConnectOpts,
    ) -> io::Result<ProxyClientStream<TcpStream>>
    where
        A: Into<Address>,
    {
        ProxyClientStream::connect_with_opts_map(context, svr_cfg, addr, opts, |s| s).await
    }
}

impl<S> ProxyClientStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`, maps `TcpStream` to customized stream with `map_fn`
    pub async fn connect_map<A, F>(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        addr: A,
        map_fn: F,
    ) -> io::Result<ProxyClientStream<S>>
    where
        A: Into<Address>,
        F: FnOnce(TcpStream) -> S,
    {
        ProxyClientStream::connect_with_opts_map(context, svr_cfg, addr, &DEFAULT_CONNECT_OPTS, map_fn).await
    }

    /// Connect to target `addr` via shadowsocks' server configured by `svr_cfg`, maps `TcpStream` to customized stream with `map_fn`
    pub async fn connect_with_opts_map<A, F>(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        addr: A,
        opts: &ConnectOpts,
        map_fn: F,
    ) -> io::Result<ProxyClientStream<S>>
    where
        A: Into<Address>,
        F: FnOnce(TcpStream) -> S,
    {
        let stream = match svr_cfg.timeout() {
            Some(d) => {
                match time::timeout(
                    d,
                    OutboundTcpStream::connect_server_with_opts(&context, svr_cfg.external_addr(), opts),
                )
                .await
                {
                    Ok(Ok(s)) => s,
                    Ok(Err(e)) => return Err(e),
                    Err(..) => {
                        return Err(io::Error::new(
                            ErrorKind::TimedOut,
                            format!("connect {} timeout", svr_cfg.addr()),
                        ))
                    }
                }
            }
            None => OutboundTcpStream::connect_server_with_opts(&context, svr_cfg.external_addr(), opts).await?,
        };

        trace!(
            "connected tcp remote {} (outbound: {}) with {:?}",
            svr_cfg.addr(),
            svr_cfg.external_addr(),
            opts
        );

        Ok(ProxyClientStream::from_stream(
            context,
            map_fn(stream.into()),
            svr_cfg,
            addr,
        ))
    }

    /// Create a `ProxyClientStream` with a connected `stream` to a shadowsocks' server
    ///
    /// NOTE: `stream` must be connected to the server with the same configuration as `svr_cfg`, otherwise strange errors would occurs
    pub fn from_stream<A>(context: SharedContext, stream: S, svr_cfg: &ServerConfig, addr: A) -> ProxyClientStream<S>
    where
        A: Into<Address>,
    {
        let addr = addr.into();
        let stream = CryptoStream::from_stream(&context, stream, svr_cfg.method(), svr_cfg.key());

        ProxyClientStream {
            stream,
            addr: Some(addr),
            context,
        }
    }

    /// Get reference to the underlying stream
    pub fn get_ref(&self) -> &S {
        self.stream.get_ref()
    }

    /// Get mutable reference to the underlying stream
    pub fn get_mut(&mut self) -> &mut S {
        self.stream.get_mut()
    }

    /// Consumes the `ProxyClientStream` and return the underlying stream
    pub fn into_inner(self) -> S {
        self.stream.into_inner()
    }
}

impl<S> AsyncRead for ProxyClientStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    #[inline]
    fn poll_read(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        let context = unsafe { &*(self.context.as_ref() as *const _) };
        self.stream.poll_read_decrypted(cx, context, buf)
    }
}

impl<S> AsyncWrite for ProxyClientStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    fn poll_write(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<Result<usize, io::Error>> {
        if self.addr.is_none() {
            // For all subsequence calls, just proxy it to self.stream
            return self.stream.poll_write_encrypted(cx, buf);
        }

        let addr = self.addr.take().unwrap();
        let addr_length = addr.serialized_len();

        let mut buffer = BytesMut::with_capacity(addr_length + buf.len());
        addr.write_to_buf(&mut buffer);
        buffer.put_slice(buf);

        ready!(self.stream.poll_write_encrypted(cx, &buffer))?;

        // NOTE:
        // poll_write will return Ok(0) if buf.len() == 0
        // But for the first call, this function will eventually send the handshake packet (IV/Salt + ADDR) to the remote address.
        //
        // https://github.com/shadowsocks/shadowsocks-rust/issues/232
        //
        // For protocols that requires *Server Hello* message, like FTP, clients won't send anything to the server until server sends handshake messages.
        // This could be achieved by calling poll_write with an empty input buffer.

        Ok(buf.len()).into()
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.stream.poll_flush(cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.stream.poll_shutdown(cx)
    }
}

impl<S> ProxyClientStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    pub fn into_split(self) -> (ProxyClientStreamReadHalf<S>, ProxyClientStreamWriteHalf<S>) {
        let (reader, writer) = self.stream.into_split();
        (
            ProxyClientStreamReadHalf {
                reader,
                context: self.context,
            },
            ProxyClientStreamWriteHalf {
                writer,
                addr: self.addr,
            },
        )
    }
}

pub struct ProxyClientStreamReadHalf<S> {
    reader: CryptoStreamReadHalf<S>,
    context: SharedContext,
}

impl<S> AsyncRead for ProxyClientStreamReadHalf<S>
where
    S: AsyncRead + Unpin,
{
    #[inline]
    fn poll_read(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        let context = unsafe { &*(self.context.as_ref() as *const _) };
        self.reader.poll_read_decrypted(cx, context, buf)
    }
}

pub struct ProxyClientStreamWriteHalf<S> {
    writer: CryptoStreamWriteHalf<S>,
    addr: Option<Address>,
}

impl<S> AsyncWrite for ProxyClientStreamWriteHalf<S>
where
    S: AsyncWrite + Unpin,
{
    fn poll_write(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<Result<usize, io::Error>> {
        if self.addr.is_none() {
            // For all subsequence calls, just proxy it to self.writer
            return self.writer.poll_write_encrypted(cx, buf);
        }

        let addr = self.addr.take().unwrap();
        let addr_length = addr.serialized_len();

        let mut buffer = BytesMut::with_capacity(addr_length + buf.len());
        addr.write_to_buf(&mut buffer);
        buffer.put_slice(buf);

        ready!(self.writer.poll_write_encrypted(cx, &buffer))?;

        // NOTE:
        // poll_write will return Ok(0) if buf.len() == 0
        // But for the first call, this function will eventually send the handshake packet (IV/Salt + ADDR) to the remote address.
        //
        // https://github.com/shadowsocks/shadowsocks-rust/issues/232
        //
        // For protocols that requires *Server Hello* message, like FTP, clients won't send anything to the server until server sends handshake messages.
        // This could be achieved by calling poll_write with an empty input buffer.

        Ok(buf.len()).into()
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.writer.poll_flush(cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.writer.poll_shutdown(cx)
    }
}
