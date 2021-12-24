//! Stream protocol implementation
use std::{
    io::{self, ErrorKind},
    marker::Unpin,
    pin::Pin,
    slice,
    task::{self, Poll},
};

use byte_string::ByteStr;
use bytes::{BufMut, Bytes, BytesMut};
use futures::ready;
use log::trace;
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use crate::{
    context::Context,
    crypto::v1::{Cipher, CipherKind},
};

enum DecryptReadState {
    WaitIv { key: Bytes },
    Read,
}

/// Reader wrapper that will decrypt data automatically
pub struct DecryptedReader {
    state: DecryptReadState,
    cipher: Option<Cipher>,
    buffer: BytesMut,
    method: CipherKind,
}

impl DecryptedReader {
    pub fn new(method: CipherKind, key: &[u8]) -> DecryptedReader {
        if method.iv_len() > 0 {
            DecryptedReader {
                state: DecryptReadState::WaitIv {
                    key: Bytes::copy_from_slice(key),
                },
                cipher: None,
                buffer: BytesMut::with_capacity(method.iv_len()),
                method,
            }
        } else {
            DecryptedReader {
                state: DecryptReadState::Read,
                cipher: Some(Cipher::new(method, key, &[])),
                buffer: BytesMut::new(),
                method,
            }
        }
    }

    /// Attempt to read decrypted data from reader
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
        loop {
            match self.state {
                DecryptReadState::WaitIv { ref key } => {
                    let key = unsafe { &*(key.as_ref() as *const _) };
                    ready!(self.poll_read_iv(cx, context, stream, key))?;

                    self.buffer.clear();
                    self.buffer.truncate(0);
                    self.state = DecryptReadState::Read;
                }
                DecryptReadState::Read => {
                    let before_n = buf.filled().len();
                    ready!(Pin::new(stream).poll_read(cx, buf))?;
                    let after_n = buf.filled().len();
                    if before_n == after_n {
                        return Ok(()).into();
                    }

                    let m = &mut buf.filled_mut()[before_n..];

                    let cipher = self.cipher.as_mut().expect("cipher is None");
                    if !cipher.decrypt_packet(m) {
                        return Err(io::Error::new(ErrorKind::Other, "stream cipher decrypt failed")).into();
                    }

                    return Ok(()).into();
                }
            }
        }
    }

    fn poll_read_iv<S>(
        &mut self,
        cx: &mut task::Context<'_>,
        context: &Context,
        stream: &mut S,
        key: &[u8],
    ) -> Poll<io::Result<()>>
    where
        S: AsyncRead + Unpin + ?Sized,
    {
        let iv_len = self.method.iv_len();

        let n = ready!(self.poll_read_exact(cx, stream, iv_len))?;
        if n < iv_len {
            return Err(ErrorKind::UnexpectedEof.into()).into();
        }

        let iv = &self.buffer[..iv_len];
        if context.check_nonce_and_set(&iv) {
            use std::io::Error;

            trace!("detected repeated stream iv {:?}", ByteStr::new(&iv));

            let err = Error::new(ErrorKind::Other, "detected repeated iv");
            return Err(err).into();
        }

        trace!("got stream iv {:?}", ByteStr::new(iv));

        let cipher = Cipher::new(self.method, key, iv);

        self.cipher = Some(cipher);

        Ok(()).into()
    }

    fn poll_read_exact<S>(&mut self, cx: &mut task::Context<'_>, stream: &mut S, size: usize) -> Poll<io::Result<usize>>
    where
        S: AsyncRead + Unpin + ?Sized,
    {
        assert!(size != 0);

        while self.buffer.len() < size {
            let remaining = size - self.buffer.len();
            let buffer = &mut self.buffer.chunk_mut()[..remaining];

            let mut read_buf =
                ReadBuf::uninit(unsafe { slice::from_raw_parts_mut(buffer.as_mut_ptr() as *mut _, remaining) });
            ready!(Pin::new(&mut *stream).poll_read(cx, &mut read_buf))?;

            let n = read_buf.filled().len();
            if n == 0 {
                if self.buffer.len() > 0 {
                    return Err(ErrorKind::UnexpectedEof.into()).into();
                } else {
                    return Ok(0).into();
                }
            }

            unsafe {
                self.buffer.advance_mut(n);
            }
        }

        Ok(size).into()
    }
}

enum EncryptWriteState {
    AssemblePacket,
    Writing { pos: usize },
}

/// Writer wrapper that will encrypt data automatically
pub struct EncryptedWriter {
    cipher: Cipher,
    buffer: BytesMut,
    state: EncryptWriteState,
}

impl EncryptedWriter {
    /// Creates a new EncryptedWriter
    pub fn new(method: CipherKind, key: &[u8], nonce: &[u8]) -> EncryptedWriter {
        // nonce should be sent with the first packet
        let mut buffer = BytesMut::with_capacity(nonce.len());
        buffer.put(nonce);

        EncryptedWriter {
            cipher: Cipher::new(method, key, nonce),
            buffer,
            state: EncryptWriteState::AssemblePacket,
        }
    }

    /// Attempt to write encrypted data into the writer
    pub fn poll_write_encrypted<S>(
        &mut self,
        cx: &mut task::Context<'_>,
        stream: &mut S,
        buf: &[u8],
    ) -> Poll<io::Result<usize>>
    where
        S: AsyncWrite + Unpin + ?Sized,
    {
        loop {
            match self.state {
                EncryptWriteState::AssemblePacket => {
                    let n = self.buffer.len();
                    self.buffer.put_slice(buf);
                    self.cipher.encrypt_packet(&mut self.buffer[n..]);
                    self.state = EncryptWriteState::Writing { pos: 0 };
                }
                EncryptWriteState::Writing { ref mut pos } => {
                    while *pos < self.buffer.len() {
                        let n = ready!(Pin::new(&mut *stream).poll_write(cx, &self.buffer[*pos..]))?;
                        if n == 0 {
                            return Err(ErrorKind::UnexpectedEof.into()).into();
                        }
                        *pos += n;
                    }

                    // Reset state
                    self.state = EncryptWriteState::AssemblePacket;
                    self.buffer.clear();

                    return Ok(buf.len()).into();
                }
            }
        }
    }
}
