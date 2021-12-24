//! Options for connecting to remote server

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos", target_os = "ios"))]
use std::ffi::OsString;
use std::net::IpAddr;

/// Options for connecting to TCP remote server
#[derive(Debug, Clone)]
pub struct TcpSocketOpts {
    /// TCP socket's `SO_SNDBUF`
    pub send_buffer_size: Option<u32>,

    /// TCP socket's `SO_RCVBUF`
    pub recv_buffer_size: Option<u32>,

    /// `TCP_NODELAY`
    pub nodelay: bool,
}

impl Default for TcpSocketOpts {
    fn default() -> TcpSocketOpts {
        TcpSocketOpts {
            send_buffer_size: None,
            recv_buffer_size: None,
            nodelay: false,
        }
    }
}

/// Options for connecting to remote server
#[derive(Debug, Clone)]
pub struct ConnectOpts {
    /// Linux mark based routing, going to set by `setsockopt` with `SO_MARK` option
    #[cfg(any(target_os = "linux", target_os = "android"))]
    pub fwmark: Option<u32>,

    /// An IPC unix socket path for sending file descriptors to call `VpnService.protect`
    ///
    /// This is an [Android shadowsocks implementation](https://github.com/shadowsocks/shadowsocks-android) specific feature
    #[cfg(target_os = "android")]
    pub vpn_protect_path: Option<std::path::PathBuf>,

    /// Outbound socket binds to this IP address, mostly for choosing network interfaces
    ///
    /// It only affects sockets that trying to connect to addresses with the same family
    pub bind_local_addr: Option<IpAddr>,

    /// Outbound socket binds to interface
    #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos", target_os = "ios"))]
    pub bind_interface: Option<OsString>,

    /// TCP options
    pub tcp: TcpSocketOpts,
}

impl Default for ConnectOpts {
    fn default() -> ConnectOpts {
        ConnectOpts {
            #[cfg(any(target_os = "linux", target_os = "android"))]
            fwmark: None,
            #[cfg(target_os = "android")]
            vpn_protect_path: None,
            bind_local_addr: None,
            #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos", target_os = "ios"))]
            bind_interface: None,
            tcp: TcpSocketOpts::default(),
        }
    }
}

/// Inbound connection options
#[derive(Clone, Debug)]
pub struct AcceptOpts {
    /// TCP options
    pub tcp: TcpSocketOpts,
}

impl Default for AcceptOpts {
    fn default() -> AcceptOpts {
        AcceptOpts {
            tcp: TcpSocketOpts::default(),
        }
    }
}
