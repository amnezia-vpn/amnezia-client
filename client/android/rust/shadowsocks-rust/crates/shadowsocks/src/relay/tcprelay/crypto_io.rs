//! IO facilities for TCP relay

use std::{
    io,
    marker::Unpin,
    pin::Pin,
    task::{self, Poll},
};

use byte_string::ByteStr;
use log::trace;
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf, ReadHalf, WriteHalf};

use crate::{
    context::Context,
    crypto::v1::{random_iv_or_salt, CipherCategory, CipherKind},
};

use super::aead::{DecryptedReader as AeadDecryptedReader, EncryptedWriter as AeadEncryptedWriter};
#[cfg(feature = "stream-cipher")]
use super::stream::{DecryptedReader as StreamDecryptedReader, EncryptedWriter as StreamEncryptedWriter};

/// Reader for reading encrypted data stream from shadowsocks' tunnel
pub enum DecryptedReader {
    None,
    Aead(AeadDecryptedReader),
    #[cfg(feature = "stream-cipher")]
    Stream(StreamDecryptedReader),
}

impl DecryptedReader {
    /// Create a new reader for reading encrypted data
    pub fn new(method: CipherKind, key: &[u8]) -> DecryptedReader {
        match method.category() {
            #[cfg(feature = "stream-cipher")]
            CipherCategory::Stream => DecryptedReader::Stream(StreamDecryptedReader::new(method, key)),
            CipherCategory::Aead => DecryptedReader::Aead(AeadDecryptedReader::new(method, key)),
            CipherCategory::None => DecryptedReader::None,
        }
    }

    /// Attempt to read decrypted data from `stream`
    #[inline]
    pub fn poll_read_decrypted<S>(
        &mut self,
        cx: &mut task::Context<'_>,
        context: &Context,
        stream: &mut S,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>>
    where
        S: AsyncRead + Unpin + ?Sized,
    {
        match *self {
            #[cfg(feature = "stream-cipher")]
            DecryptedReader::Stream(ref mut reader) => reader.poll_read_decrypted(cx, context, stream, buf),
            DecryptedReader::Aead(ref mut reader) => reader.poll_read_decrypted(cx, context, stream, buf),
            DecryptedReader::None => Pin::new(stream).poll_read(cx, buf),
        }
    }
}

/// Writer for writing encrypted data stream into shadowsocks' tunnel
pub enum EncryptedWriter {
    None,
    Aead(AeadEncryptedWriter),
    #[cfg(feature = "stream-cipher")]
    Stream(StreamEncryptedWriter),
}

impl EncryptedWriter {
    /// Create a new writer for writing encrypted data
    pub fn new(method: CipherKind, key: &[u8], nonce: &[u8]) -> EncryptedWriter {
        match method.category() {
            #[cfg(feature = "stream-cipher")]
            CipherCategory::Stream => EncryptedWriter::Stream(StreamEncryptedWriter::new(method, key, nonce)),
            CipherCategory::Aead => EncryptedWriter::Aead(AeadEncryptedWriter::new(method, key, nonce)),
            CipherCategory::None => EncryptedWriter::None,
        }
    }

    /// Attempt to write encrypted data to `stream`
    #[inline]
    pub fn poll_write_encrypted<S>(
        &mut self,
        cx: &mut task::Context<'_>,
        stream: &mut S,
        buf: &[u8],
    ) -> Poll<io::Result<usize>>
    where
        S: AsyncWrite + Unpin + ?Sized,
    {
        match *self {
            #[cfg(feature = "stream-cipher")]
            EncryptedWriter::Stream(ref mut writer) => writer.poll_write_encrypted(cx, stream, buf),
            EncryptedWriter::Aead(ref mut writer) => writer.poll_write_encrypted(cx, stream, buf),
            EncryptedWriter::None => Pin::new(stream).poll_write(cx, buf),
        }
    }
}

/// A bidirectional stream for read/write encrypted data in shadowsocks' tunnel
pub struct CryptoStream<S> {
    stream: S,
    dec: DecryptedReader,
    enc: EncryptedWriter,
    method: CipherKind,
}

impl<S> CryptoStream<S> {
    /// Create a new CryptoStream with the underlying stream connection
    pub fn from_stream(context: &Context, stream: S, method: CipherKind, key: &[u8]) -> CryptoStream<S> {
        let category = method.category();

        if category == CipherCategory::None {
            // Fast-path for none cipher
            return CryptoStream::<S>::new_none(stream, method);
        }

        let prev_len = match category {
            #[cfg(feature = "stream-cipher")]
            CipherCategory::Stream => method.iv_len(),
            CipherCategory::Aead => method.salt_len(),
            CipherCategory::None => 0,
        };

        let iv = match category {
            #[cfg(feature = "stream-cipher")]
            CipherCategory::Stream => {
                let local_iv = loop {
                    let mut iv = vec![0u8; prev_len];
                    if prev_len > 0 {
                        random_iv_or_salt(&mut iv);
                    }

                    if context.check_nonce_and_set(&iv) {
                        // IV exist, generate another one
                        continue;
                    }
                    break iv;
                };
                trace!("generated Stream cipher IV {:?}", ByteStr::new(&local_iv));
                local_iv
            }
            CipherCategory::Aead => {
                let local_salt = loop {
                    let mut salt = vec![0u8; prev_len];
                    if prev_len > 0 {
                        random_iv_or_salt(&mut salt);
                    }

                    if context.check_nonce_and_set(&salt) {
                        // Salt exist, generate another one
                        continue;
                    }
                    break salt;
                };
                trace!("generated AEAD cipher salt {:?}", ByteStr::new(&local_salt));
                local_salt
            }
            CipherCategory::None => Vec::new(),
        };

        CryptoStream {
            stream,
            dec: DecryptedReader::new(method, key),
            enc: EncryptedWriter::new(method, key, &iv),
            method,
        }
    }

    fn new_none(stream: S, method: CipherKind) -> CryptoStream<S> {
        CryptoStream {
            stream,
            dec: DecryptedReader::None,
            enc: EncryptedWriter::None,
            method,
        }
    }

    /// Return a reference to the underlying stream
    pub fn get_ref(&self) -> &S {
        &self.stream
    }

    /// Return a mutable reference to the underlying stream
    pub fn get_mut(&mut self) -> &mut S {
        &mut self.stream
    }

    /// Consume the CryptoStream and return the internal stream instance
    pub fn into_inner(self) -> S {
        self.stream
    }
}

impl<S> CryptoStream<S> {
    /// Get encryption method
    pub fn method(&self) -> CipherKind {
        self.method
    }
}

impl<S> CryptoStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    /// Attempt to read decrypted data from `stream`
    #[inline]
    pub fn poll_read_decrypted(
        &mut self,
        cx: &mut task::Context<'_>,
        context: &Context,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        self.dec.poll_read_decrypted(cx, context, &mut self.stream, buf)
    }
}

impl<S> CryptoStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    /// Attempt to write encrypted data to `stream`
    #[inline]
    pub fn poll_write_encrypted(&mut self, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        self.enc.poll_write_encrypted(cx, &mut self.stream, buf)
    }

    /// Polls `flush` on the underlying stream
    #[inline]
    pub fn poll_flush(&mut self, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.stream).poll_flush(cx)
    }

    /// Polls `shutdown` on the underlying stream
    #[inline]
    pub fn poll_shutdown(&mut self, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.stream).poll_shutdown(cx)
    }
}

impl<S> CryptoStream<S>
where
    S: AsyncRead + AsyncWrite + Unpin,
{
    pub fn into_split(self) -> (CryptoStreamReadHalf<S>, CryptoStreamWriteHalf<S>) {
        let (reader, writer) = tokio::io::split(self.stream);

        (
            CryptoStreamReadHalf {
                reader,
                dec: self.dec,
                method: self.method,
            },
            CryptoStreamWriteHalf {
                writer,
                enc: self.enc,
                method: self.method,
            },
        )
    }
}

pub struct CryptoStreamReadHalf<S> {
    reader: ReadHalf<S>,
    dec: DecryptedReader,
    method: CipherKind,
}

impl<S> CryptoStreamReadHalf<S> {
    /// Get encryption method
    pub fn method(&self) -> CipherKind {
        self.method
    }
}

impl<S> CryptoStreamReadHalf<S>
where
    S: AsyncRead + Unpin,
{
    /// Attempt to read decrypted data from `stream`
    #[inline]
    pub fn poll_read_decrypted(
        &mut self,
        cx: &mut task::Context<'_>,
        context: &Context,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        self.dec.poll_read_decrypted(cx, context, &mut self.reader, buf)
    }
}

pub struct CryptoStreamWriteHalf<S> {
    writer: WriteHalf<S>,
    enc: EncryptedWriter,
    method: CipherKind,
}

impl<S> CryptoStreamWriteHalf<S> {
    /// Get encryption method
    pub fn method(&self) -> CipherKind {
        self.method
    }
}

impl<S> CryptoStreamWriteHalf<S>
where
    S: AsyncWrite + Unpin,
{
    /// Attempt to write encrypted data to `stream`
    #[inline]
    pub fn poll_write_encrypted(&mut self, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        self.enc.poll_write_encrypted(cx, &mut self.writer, buf)
    }

    /// Polls `flush` on the underlying stream
    #[inline]
    pub fn poll_flush(&mut self, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.writer).poll_flush(cx)
    }

    /// Polls `shutdown` on the underlying stream
    #[inline]
    pub fn poll_shutdown(&mut self, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.writer).poll_shutdown(cx)
    }
}
