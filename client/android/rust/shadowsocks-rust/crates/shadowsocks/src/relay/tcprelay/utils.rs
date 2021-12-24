//! Utilities for TCP relay

use std::{
    future::Future,
    io,
    pin::Pin,
    task::{Context, Poll},
};

use futures::ready;
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use crate::crypto::v1::{CipherCategory, CipherKind};

/// A future that asynchronously copies the entire contents of a reader into a
/// writer.
#[derive(Debug)]
#[must_use = "futures do nothing unless you `.await` or poll them"]
struct Copy<'a, R: ?Sized, W: ?Sized> {
    reader: &'a mut R,
    read_done: bool,
    writer: &'a mut W,
    pos: usize,
    cap: usize,
    amt: u64,
    buf: Box<[u8]>,
}

impl<R, W> Future for Copy<'_, R, W>
where
    R: AsyncRead + Unpin + ?Sized,
    W: AsyncWrite + Unpin + ?Sized,
{
    type Output = io::Result<u64>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<u64>> {
        loop {
            // If our buffer is empty, then we need to read some data to
            // continue.
            if self.pos == self.cap && !self.read_done {
                let me = &mut *self;
                let mut buf = ReadBuf::new(&mut me.buf);
                ready!(Pin::new(&mut *me.reader).poll_read(cx, &mut buf))?;
                let n = buf.filled().len();
                if n == 0 {
                    self.read_done = true;
                } else {
                    self.pos = 0;
                    self.cap = n;
                }
            }

            // If our buffer has some data, let's write it out!
            while self.pos < self.cap {
                let me = &mut *self;
                let i = ready!(Pin::new(&mut *me.writer).poll_write(cx, &me.buf[me.pos..me.cap]))?;
                if i == 0 {
                    return Poll::Ready(Err(io::Error::new(
                        io::ErrorKind::WriteZero,
                        "write zero byte into writer",
                    )));
                } else {
                    self.pos += i;
                    self.amt += i as u64;
                }
            }

            // If we've written all the data and we've seen EOF, flush out the
            // data and finish the transfer.
            if self.pos == self.cap && self.read_done {
                let me = &mut *self;
                ready!(Pin::new(&mut *me.writer).poll_flush(cx))?;
                return Poll::Ready(Ok(self.amt));
            }
        }
    }
}

/// Copy data from encrypted reader to plain writer
pub async fn copy_from_encrypted<ER, PW>(method: CipherKind, reader: &mut ER, writer: &mut PW) -> io::Result<u64>
where
    ER: AsyncRead + Unpin + ?Sized,
    PW: AsyncWrite + Unpin + ?Sized,
{
    Copy {
        reader,
        read_done: false,
        writer,
        amt: 0,
        pos: 0,
        cap: 0,
        buf: alloc_encrypted_read_buffer(method),
    }
    .await
}

/// Copy data from plain reader to encrypted writer
pub async fn copy_to_encrypted<PR, EW>(method: CipherKind, reader: &mut PR, writer: &mut EW) -> io::Result<u64>
where
    PR: AsyncRead + Unpin + ?Sized,
    EW: AsyncWrite + Unpin + ?Sized,
{
    Copy {
        reader,
        read_done: false,
        writer,
        amt: 0,
        pos: 0,
        cap: 0,
        buf: alloc_plain_read_buffer(method),
    }
    .await
}

/// Create a buffer for reading from shadowsocks' encrypted channel
pub fn alloc_encrypted_read_buffer(method: CipherKind) -> Box<[u8]> {
    match method.category() {
        CipherCategory::Aead => vec![0u8; super::aead::MAX_PACKET_SIZE + method.tag_len()].into_boxed_slice(),
        #[cfg(feature = "stream-cipher")]
        CipherCategory::Stream => vec![0u8; 1 << 14].into_boxed_slice(),
        CipherCategory::None => vec![0u8; 1 << 14].into_boxed_slice(),
    }
}

/// Create a buffer for reading from plain channel (not encrypted), for copying data into encrypted channel
pub fn alloc_plain_read_buffer(method: CipherKind) -> Box<[u8]> {
    match method.category() {
        CipherCategory::Aead => vec![0u8; super::aead::MAX_PACKET_SIZE].into_boxed_slice(),
        #[cfg(feature = "stream-cipher")]
        CipherCategory::Stream => vec![0u8; 1 << 14].into_boxed_slice(),
        CipherCategory::None => vec![0u8; 1 << 14].into_boxed_slice(),
    }
}
