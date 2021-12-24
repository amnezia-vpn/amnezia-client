//! UDP relay client

use std::io::{self, Cursor, ErrorKind};

use bytes::{BufMut, BytesMut};
use tokio::net::{ToSocketAddrs, UdpSocket};

use shadowsocks::relay::socks5::{Address, Error, UdpAssociateHeader};

use super::tcp_client::Socks5TcpClient;

/// Socks5 proxy client
pub struct Socks5UdpClient {
    socket: UdpSocket,
    // Socks5 protocol requires to keep this TCP connection alive
    // Theoretically if this connection is broken, the association is broken too, but the UDP Socks5 server in this crate doesn't behave like that
    #[allow(dead_code)]
    assoc_client: Option<Socks5TcpClient>,
}

impl Socks5UdpClient {
    /// Create a new UDP associate client binds to a specific address
    pub async fn bind<A>(addrs: A) -> io::Result<Socks5UdpClient>
    where
        A: ToSocketAddrs,
    {
        Ok(Socks5UdpClient {
            socket: UdpSocket::bind(addrs).await?,
            assoc_client: None,
        })
    }

    /// Create a new UDP associate to `proxy`
    pub async fn associate<P>(&mut self, proxy: P) -> Result<(), Error>
    where
        P: ToSocketAddrs,
    {
        if self.assoc_client.is_some() {
            let err = io::Error::new(ErrorKind::Other, "udp is associated");
            return Err(err.into());
        }

        // The actual bind address, tell the proxy that I am going to send packets from this address
        let local_addr = self.socket.local_addr()?;

        let (assoc_client, proxy_addr) = Socks5TcpClient::udp_associate(local_addr, proxy).await?;
        match proxy_addr {
            Address::SocketAddress(sa) => self.socket.connect(sa).await?,
            // FIXME: `connect` will use tokio's builtin DNS resolver.
            // But if we want to use `trust-dns`, we have to initialize a `Context` instance (for the global `AsyncResolver` instance)
            Address::DomainNameAddress(ref dname, port) => self.socket.connect((dname.as_str(), port)).await?,
        }

        self.assoc_client = Some(assoc_client);

        Ok(())
    }

    /// Returns a future that sends data on the socket to the given address.
    pub async fn send_to<A>(&self, frag: u8, buf: &[u8], target: A) -> Result<usize, Error>
    where
        A: Into<Address>,
    {
        self.check_associated()?;

        let header = UdpAssociateHeader::new(frag, target.into());
        let header_len = header.serialized_len();
        let mut send_buf = BytesMut::with_capacity(header.serialized_len() + buf.len());
        header.write_to_buf(&mut send_buf);
        send_buf.put_slice(buf);

        let n = self.socket.send(&send_buf).await?;
        Ok(if n <= header_len { 0 } else { n - header_len })
    }

    /// Returns a future that receives a single datagram on the socket. On success, the future resolves to the number of bytes read and the origin.
    ///
    /// The function must be called with valid byte array buf of sufficient size to hold the message bytes.
    /// If a message is too long to fit in the supplied buffer, excess bytes may be discarded.
    pub async fn recv_from(&self, recv_buf: &mut [u8]) -> Result<(usize, u8, Address), Error> {
        self.check_associated()?;

        let n = self.socket.recv(recv_buf).await?;

        // Address + Payload
        let mut cur = Cursor::new(&recv_buf[..n]);

        let header = UdpAssociateHeader::read_from(&mut cur).await?;
        let pos = cur.position() as usize;

        recv_buf.copy_within(pos.., 0);

        Ok((n - pos, header.frag, header.address))
    }

    fn check_associated(&self) -> io::Result<()> {
        if self.assoc_client.is_none() {
            let err = io::Error::new(ErrorKind::Other, "udp not associated");
            return Err(err.into());
        }
        Ok(())
    }
}
