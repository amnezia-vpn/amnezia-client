//! DNS configurations

#[cfg(unix)]
use std::{convert::Infallible, path::PathBuf};
use std::{
    fmt::{self, Display},
    net::SocketAddr,
    str::FromStr,
};

/// DNS name server address
#[derive(Debug, Clone, Eq, PartialEq, Hash)]
pub enum NameServerAddr {
    /// IP address
    SocketAddr(SocketAddr),
    /// Unix Domain Socket address
    ///
    /// Specifically used by Android, which served as a stream protocol based DNS server
    #[cfg(unix)]
    UnixSocketAddr(PathBuf),
}

/// Parse `NameServerAddr` error
#[cfg(unix)]
pub type NameServerAddrError = Infallible;
/// Parse `NameServerAddr` error
#[cfg(not(unix))]
pub type NameServerAddrError = <SocketAddr as FromStr>::Err;

impl FromStr for NameServerAddr {
    type Err = NameServerAddrError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s.parse::<SocketAddr>() {
            Ok(addr) => Ok(NameServerAddr::SocketAddr(addr)),
            #[cfg(unix)]
            Err(..) => Ok(NameServerAddr::UnixSocketAddr(PathBuf::from(s))),
            #[cfg(not(unix))]
            Err(err) => Err(err),
        }
    }
}

impl Display for NameServerAddr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            NameServerAddr::SocketAddr(ref sa) => Display::fmt(sa, f),
            #[cfg(unix)]
            NameServerAddr::UnixSocketAddr(ref p) => write!(f, "{}", p.display()),
        }
    }
}
