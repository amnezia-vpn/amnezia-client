//! A TCP stream for communicating with shadowsocks' proxy client

use std::{
    io,
    pin::Pin,
    task::{self, Poll},
};

use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use crate::{
    context::SharedContext,
    crypto::v1::CipherKind,
    relay::tcprelay::crypto_io::{CryptoStream, CryptoStreamReadHalf, CryptoStreamWriteHalf},
};

/// A stream for communicating with shadowsocks' proxy client
pub struct ProxyServerStream<S> {
    stream: CryptoStream<S>,
    context: SharedContext,
}

impl<S> ProxyServerStream<S> {
    pub(crate) fn from_stream(
        context: SharedContext,
        stream: S,
        method: CipherKind,
        key: &[u8],
    ) -> ProxyServerStream<S> {
        ProxyServerStream {
            stream: CryptoStream::from_stream(&context, stream, method, key),
            context,
        }
    }

    /// Get reference of the internal stream
    pub fn get_ref(&self) -> &S {
        self.stream.get_ref()
    }

    /// Get mutable reference of the internal stream
    pub fn get_mut(&mut self) -> &mut S {
        self.stream.get_mut()
    }

    /// Consumes the object and return the internal stream
    pub fn into_inner(self) -> S {
        self.stream.into_inner()
    }
}

impl<S> ProxyServerStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    /// Splits into reader and writer halves
    pub fn into_split(self) -> (ProxyServerStreamReadHalf<S>, ProxyServerStreamWriteHalf<S>) {
        let (reader, writer) = self.stream.into_split();

        (
            ProxyServerStreamReadHalf {
                reader,
                context: self.context,
            },
            ProxyServerStreamWriteHalf { writer },
        )
    }
}

impl<S> AsyncRead for ProxyServerStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    fn poll_read(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        let context = unsafe { &*(self.context.as_ref() as *const _) };
        self.stream.poll_read_decrypted(cx, context, buf)
    }
}

impl<S> AsyncWrite for ProxyServerStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    fn poll_write(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<Result<usize, io::Error>> {
        self.stream.poll_write_encrypted(cx, buf)
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.stream.poll_flush(cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.stream.poll_shutdown(cx)
    }
}

pub struct ProxyServerStreamReadHalf<S> {
    reader: CryptoStreamReadHalf<S>,
    context: SharedContext,
}

impl<S> AsyncRead for ProxyServerStreamReadHalf<S>
where
    S: AsyncRead + Unpin,
{
    fn poll_read(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        let context = unsafe { &*(self.context.as_ref() as *const _) };
        self.reader.poll_read_decrypted(cx, context, buf)
    }
}

pub struct ProxyServerStreamWriteHalf<S> {
    writer: CryptoStreamWriteHalf<S>,
}

impl<S> AsyncWrite for ProxyServerStreamWriteHalf<S>
where
    S: AsyncWrite + Unpin,
{
    fn poll_write(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<Result<usize, io::Error>> {
        self.writer.poll_write_encrypted(cx, buf)
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.writer.poll_flush(cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<Result<(), io::Error>> {
        self.writer.poll_shutdown(cx)
    }
}
