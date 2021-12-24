use std::{
    io::{self, Error, ErrorKind},
    net::SocketAddr,
};

use async_trait::async_trait;
use socket2::Protocol;
use tokio::net::{TcpListener, TcpStream};

use crate::{
    config::RedirType,
    local::redir::redir_ext::{TcpListenerRedirExt, TcpStreamRedirExt},
};

#[async_trait]
impl TcpListenerRedirExt for TcpListener {
    async fn bind_redir(ty: RedirType, addr: SocketAddr) -> io::Result<TcpListener> {
        match ty {
            #[cfg(any(
                target_os = "openbsd",
                target_os = "freebsd",
                target_os = "netbsd",
                target_os = "solaris",
                target_os = "macos",
                target_os = "ios",
            ))]
            RedirType::PacketFilter => {}

            #[cfg(any(
                target_os = "freebsd",
                target_os = "macos",
                target_os = "ios",
                target_os = "dragonfly"
            ))]
            RedirType::IpFirewall => {}

            _ => {
                return Err(Error::new(
                    ErrorKind::InvalidInput,
                    "not supported tcp transparent proxy type",
                ));
            }
        }

        // BSD platform doesn't have any special logic
        TcpListener::bind(addr).await
    }
}

impl TcpStreamRedirExt for TcpStream {
    fn destination_addr(&self, ty: RedirType) -> io::Result<SocketAddr> {
        match ty {
            #[cfg(any(
                target_os = "openbsd",
                target_os = "freebsd",
                target_os = "netbsd",
                target_os = "solaris",
                target_os = "macos",
                target_os = "ios",
            ))]
            RedirType::PacketFilter => {
                use crate::local::redir::sys::bsd_pf::PF;

                let peer_addr = self.peer_addr()?;
                let bind_addr = self.local_addr()?;

                PF.natlook(&bind_addr, &peer_addr, Protocol::TCP)
            }
            #[cfg(any(
                target_os = "freebsd",
                target_os = "macos",
                target_os = "ios",
                target_os = "dragonfly"
            ))]
            RedirType::IpFirewall => {
                // ## IPFW
                //
                // For IPFW, uses getsockname() to retrieve destination address
                //
                // FreeBSD: https://www.freebsd.org/doc/handbook/firewalls-ipfw.html
                self.local_addr()
            }
            _ => unreachable!("not supported tcp transparent proxy type"),
        }
    }
}
