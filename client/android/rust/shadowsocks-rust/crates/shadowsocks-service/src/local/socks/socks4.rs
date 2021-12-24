//! Socks4a Protocol Definition
//!
//! <http://ftp.icm.edu.pl/packages/socks/socks4/SOCKS4.protocol>

#![allow(dead_code)]

use std::{
    fmt,
    io::{self, ErrorKind},
    net::{Ipv4Addr, SocketAddr, SocketAddrV4},
};

use byteorder::{BigEndian, ByteOrder};
use bytes::{BufMut, BytesMut};
use thiserror::Error;
use tokio::io::{AsyncBufRead, AsyncBufReadExt, AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt};

use shadowsocks::relay::socks5;

#[rustfmt::skip]
mod consts {
    pub const SOCKS4_VERSION:                                  u8 = 4;

    pub const SOCKS4_COMMAND_CONNECT:                          u8 = 1;
    pub const SOCKS4_COMMAND_BIND:                             u8 = 2;

    pub const SOCKS4_RESULT_REQUEST_GRANTED:                   u8 = 90;
    pub const SOCKS4_RESULT_REQUEST_REJECTED_OR_FAILED:        u8 = 91;
    pub const SOCKS4_RESULT_REQUEST_REJECTED_CANNOT_CONNECT:   u8 = 92;
    pub const SOCKS4_RESULT_REQUEST_REJECTED_DFFERENT_USER_ID: u8 = 93;
}

/// SOCKS4 Command
#[derive(Clone, Debug, Copy)]
pub enum Command {
    /// CONNECT command
    Connect,
    /// BIND command
    Bind,
}

impl Command {
    #[inline]
    fn as_u8(self) -> u8 {
        match self {
            Command::Connect => consts::SOCKS4_COMMAND_CONNECT,
            Command::Bind => consts::SOCKS4_COMMAND_BIND,
        }
    }

    #[inline]
    fn from_u8(code: u8) -> Option<Command> {
        match code {
            consts::SOCKS4_COMMAND_CONNECT => Some(Command::Connect),
            consts::SOCKS4_COMMAND_BIND => Some(Command::Bind),
            _ => None,
        }
    }
}

/// SOCKS4 Result Code
#[derive(Clone, Debug, Copy, Eq, PartialEq)]
pub enum ResultCode {
    /// 90: request granted
    RequestGranted,
    /// 91: request rejected or failed
    RequestRejectedOrFailed,
    /// 92: request rejected becasue SOCKS server cannot connect to identd on the client
    RequestRejectedCannotConnect,
    /// 93: request rejected because the client program and identd report different user-ids
    RequestRejectedDifferentUserId,
    /// Other replies
    Other(u8),
}

impl ResultCode {
    #[inline]
    fn as_u8(self) -> u8 {
        match self {
            ResultCode::RequestGranted => consts::SOCKS4_RESULT_REQUEST_GRANTED,
            ResultCode::RequestRejectedOrFailed => consts::SOCKS4_RESULT_REQUEST_REJECTED_OR_FAILED,
            ResultCode::RequestRejectedCannotConnect => consts::SOCKS4_RESULT_REQUEST_REJECTED_CANNOT_CONNECT,
            ResultCode::RequestRejectedDifferentUserId => consts::SOCKS4_RESULT_REQUEST_REJECTED_DFFERENT_USER_ID,
            ResultCode::Other(c) => c,
        }
    }

    #[inline]
    fn from_u8(code: u8) -> ResultCode {
        match code {
            consts::SOCKS4_RESULT_REQUEST_GRANTED => ResultCode::RequestGranted,
            consts::SOCKS4_RESULT_REQUEST_REJECTED_OR_FAILED => ResultCode::RequestRejectedOrFailed,
            consts::SOCKS4_RESULT_REQUEST_REJECTED_CANNOT_CONNECT => ResultCode::RequestRejectedCannotConnect,
            consts::SOCKS4_RESULT_REQUEST_REJECTED_DFFERENT_USER_ID => ResultCode::RequestRejectedDifferentUserId,
            code => ResultCode::Other(code),
        }
    }
}

impl fmt::Display for ResultCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            ResultCode::RequestGranted => f.write_str("request granted"),
            ResultCode::RequestRejectedOrFailed => f.write_str("request rejected or failed"),
            ResultCode::RequestRejectedCannotConnect => {
                f.write_str("request rejected becasue SOCKS server cannot connect to identd on the client")
            }
            ResultCode::RequestRejectedDifferentUserId => {
                f.write_str("request rejected because the client program and identd report different user-ids")
            }
            ResultCode::Other(code) => write!(f, "other result code {}", code),
        }
    }
}

/// SOCKS4 Address type
#[derive(Clone, PartialEq, Eq, Hash)]
pub enum Address {
    /// Socket address (IP Address)
    SocketAddress(SocketAddrV4),
    /// Domain name address (SOCKS4a)
    DomainNameAddress(String, u16),
}

impl fmt::Debug for Address {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Address::SocketAddress(ref addr) => write!(f, "{}", addr),
            Address::DomainNameAddress(ref addr, ref port) => write!(f, "{}:{}", addr, port),
        }
    }
}

impl fmt::Display for Address {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Address::SocketAddress(ref addr) => write!(f, "{}", addr),
            Address::DomainNameAddress(ref addr, ref port) => write!(f, "{}:{}", addr, port),
        }
    }
}

impl From<SocketAddrV4> for Address {
    fn from(s: SocketAddrV4) -> Address {
        Address::SocketAddress(s)
    }
}

impl From<(String, u16)> for Address {
    fn from((dn, port): (String, u16)) -> Address {
        Address::DomainNameAddress(dn, port)
    }
}

impl From<(&str, u16)> for Address {
    fn from((dn, port): (&str, u16)) -> Address {
        Address::DomainNameAddress(dn.to_owned(), port)
    }
}

impl From<&Address> for Address {
    fn from(addr: &Address) -> Address {
        addr.clone()
    }
}

impl From<Address> for socks5::Address {
    fn from(addr: Address) -> socks5::Address {
        match addr {
            Address::SocketAddress(a) => socks5::Address::SocketAddress(SocketAddr::V4(a)),
            Address::DomainNameAddress(d, p) => socks5::Address::DomainNameAddress(d, p),
        }
    }
}

/// Handshake Request
///
/// ```plain
/// The client connects to the SOCKS server and sends a CONNECT/BIND request when
/// it wants to establish a connection to an application server. The client
/// includes in the request packet the IP address and the port number of the
/// destination host, and userid, in the following format.
///
///                 +----+----+----+----+----+----+----+----+----+----+....+----+
///                 | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
///                 +----+----+----+----+----+----+----+----+----+----+....+----+
///  # of bytes:	   1    1      2              4           variable       1
///
/// VN is the SOCKS protocol version number and should be 4. CD is the
/// SOCKS command code and should be 1 for CONNECT request, 2 for BIND request. NULL is a byte
/// of all zero bits.
/// ```
#[derive(Debug, Clone)]
pub struct HandshakeRequest {
    pub cd: Command,
    pub dst: Address,
    pub user_id: Vec<u8>,
}

impl HandshakeRequest {
    /// Read from a reader
    pub async fn read_from<R>(r: &mut R) -> Result<HandshakeRequest, Error>
    where
        R: AsyncBufRead + Unpin,
    {
        let mut buf = [0u8; 8];
        let _ = r.read_exact(&mut buf).await?;

        let vn = buf[0];
        if vn != consts::SOCKS4_VERSION {
            return Err(Error::UnsupportedSocksVersion(vn));
        }

        let cd = buf[1];
        let command = match Command::from_u8(cd) {
            Some(c) => c,
            None => {
                return Err(Error::UnsupportedSocksVersion(cd));
            }
        };

        let port = BigEndian::read_u16(&buf[2..4]);

        let mut user_id = Vec::new();
        let _ = r.read_until(b'\0', &mut user_id).await?;
        if user_id.is_empty() || user_id.last() != Some(&b'\0') {
            return Err(io::Error::from(ErrorKind::UnexpectedEof).into());
        }
        user_id.pop(); // Pops the last b'\0'

        let dst = if buf[4] == 0x00 && buf[5] == 0x00 && buf[6] == 0x00 && buf[7] != 0x00 {
            // SOCKS4a, indicates that it is a HOST address
            let mut host = Vec::new();
            let _ = r.read_until(b'\0', &mut host).await?;
            if host.is_empty() || host.last() != Some(&b'\0') {
                return Err(io::Error::from(ErrorKind::UnexpectedEof).into());
            }
            host.pop(); // Pops the last b'\0'

            match String::from_utf8(host) {
                Ok(host) => Address::DomainNameAddress(host, port),
                Err(..) => {
                    return Err(Error::AddressHostInvalidEncoding);
                }
            }
        } else {
            let ip = Ipv4Addr::new(buf[4], buf[5], buf[6], buf[7]);
            Address::SocketAddress(SocketAddrV4::new(ip, port))
        };

        Ok(HandshakeRequest {
            cd: command,
            dst,
            user_id,
        })
    }

    /// Writes to writer
    pub async fn write_to<W>(&self, w: &mut W) -> io::Result<()>
    where
        W: AsyncWrite + Unpin,
    {
        let mut buf = BytesMut::with_capacity(self.serialized_len());
        self.write_to_buf(&mut buf);
        w.write_all(&buf).await
    }

    /// Writes to buffer
    pub fn write_to_buf<B: BufMut>(&self, buf: &mut B) {
        debug_assert!(
            !self.user_id.contains(&b'\0'),
            "USERID shouldn't contain any NULL characters"
        );

        buf.put_u8(consts::SOCKS4_VERSION);
        buf.put_u8(self.cd.as_u8());
        match self.dst {
            Address::SocketAddress(ref saddr) => {
                let port = saddr.port();
                buf.put_u16(port);
                buf.put_slice(&saddr.ip().octets());

                buf.put_slice(&self.user_id);
                buf.put_u8(b'\0');
            }
            Address::DomainNameAddress(ref dname, port) => {
                buf.put_u16(port);

                // 0.0.0.x (x != 0)
                const PLACEHOLDER: [u8; 4] = [0x00, 0x00, 0x00, 0xff];
                buf.put_slice(&PLACEHOLDER);

                buf.put_slice(&self.user_id);
                buf.put_u8(b'\0');

                buf.put_slice(dname.as_bytes());
                buf.put_u8(b'\0');
            }
        }
    }

    /// Length in bytes
    #[inline]
    pub fn serialized_len(&self) -> usize {
        let mut s = 1 + 1 + 2 + 4 + self.user_id.len() + 1; // USERID.LEN + NULL
        if let Address::DomainNameAddress(ref dname, _) = self.dst {
            s += dname.len() + 1;
        }
        s
    }
}

/// Handshake Response
///
/// ```plain
///             +----+----+----+----+----+----+----+----+
///             | VN | CD | DSTPORT |      DSTIP        |
///             +----+----+----+----+----+----+----+----+
/// # of bytes:	   1    1      2              4
/// ```
#[derive(Debug, Clone)]
pub struct HandshakeResponse {
    pub cd: ResultCode,
}

impl HandshakeResponse {
    /// Create a response with code
    pub fn new(code: ResultCode) -> HandshakeResponse {
        HandshakeResponse { cd: code }
    }

    /// Read from a reader
    pub async fn read_from<R>(r: &mut R) -> Result<HandshakeResponse, Error>
    where
        R: AsyncRead + Unpin,
    {
        let mut buf = [0u8; 8];
        let _ = r.read_exact(&mut buf).await?;

        let vn = buf[0];
        if vn != 0 {
            return Err(Error::UnsupportedSocksVersion(vn));
        }

        let cd = buf[1];
        let result_code = ResultCode::from_u8(cd);

        // DSTPORT, DSTIP are ignored

        Ok(HandshakeResponse { cd: result_code })
    }

    /// Write data into a writer
    pub async fn write_to<W>(&self, w: &mut W) -> io::Result<()>
    where
        W: AsyncWrite + Unpin,
    {
        let mut buf = BytesMut::with_capacity(self.serialized_len());
        self.write_to_buf(&mut buf);
        w.write_all(&buf).await
    }

    /// Writes to buffer
    pub fn write_to_buf<B: BufMut>(&self, buf: &mut B) {
        let HandshakeResponse { ref cd } = *self;

        buf.put_slice(&[
            // VN: Result Code's version, must be 0
            0x00,
            // CD: Result Code
            cd.as_u8(),
            // DSTPORT: Ignored
            0x00,
            0x00,
            // DSTIP: Ignored
            0x00,
            0x00,
            0x00,
            0x00,
        ]);
    }

    /// Length in bytes
    #[inline]
    pub fn serialized_len(&self) -> usize {
        1 + 1 + 2 + 4
    }
}

/// SOCKS 4/4a Error
#[derive(Error, Debug)]
pub enum Error {
    // I/O Error
    #[error("{0}")]
    IoError(#[from] io::Error),
    #[error("host must be UTF-8 encoding")]
    AddressHostInvalidEncoding,
    #[error("unsupported socks version {0:#x}")]
    UnsupportedSocksVersion(u8),
    #[error("unsupported command {0:#x}")]
    UnsupportedCommand(u8),
    #[error("{0}")]
    Result(ResultCode),
}

impl From<Error> for io::Error {
    fn from(err: Error) -> io::Error {
        match err {
            Error::IoError(err) => err,
            e => io::Error::new(ErrorKind::Other, e),
        }
    }
}
