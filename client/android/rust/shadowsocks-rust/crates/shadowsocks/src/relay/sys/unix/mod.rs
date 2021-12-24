#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos", target_os = "ios"))]
use std::os::unix::io::AsRawFd;
use std::{
    io::{self, Error, ErrorKind},
    mem,
    net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr},
};
#[cfg(any(target_os = "android"))]
use std::{os::unix::io::RawFd, path::Path};
#[cfg(any(target_os = "macos", target_os = "ios"))]
use std::{os::unix::prelude::OsStrExt, ptr};

use cfg_if::cfg_if;
use log::{debug, warn};
use socket2::{Domain, Protocol, SockAddr, Socket, Type};
use tokio::net::{TcpSocket, TcpStream, UdpSocket};

use crate::net::{AddrFamily, ConnectOpts};

cfg_if! {
    if #[cfg(target_os = "android")] {
        mod uds;

        /// This is a RPC for Android to `protect()` socket for connecting to remote servers
        ///
        /// https://developer.android.com/reference/android/net/VpnService#protect(java.net.Socket)
        ///
        /// More detail could be found in [shadowsocks-android](https://github.com/shadowsocks/shadowsocks-android) project.
        async fn protect<P: AsRef<Path>>(protect_path: P, fd: RawFd) -> io::Result<()> {
            use tokio::io::AsyncReadExt;

            let mut stream = self::uds::UnixStream::connect(protect_path).await?;

            // send fds
            let dummy: [u8; 1] = [1];
            let fds: [RawFd; 1] = [fd];
            stream.send_with_fd(&dummy, &fds).await?;

            // receive the return value
            let mut response = [0; 1];
            stream.read_exact(&mut response).await?;

            if response[0] == 0xFF {
                return Err(Error::new(ErrorKind::Other, "protect() failed"));
            }

            Ok(())
        }
    }
}

/// create a new TCP stream
#[inline(always)]
#[allow(unused_variables)]
pub async fn tcp_stream_connect(saddr: &SocketAddr, config: &ConnectOpts) -> io::Result<TcpStream> {
    let socket = match *saddr {
        SocketAddr::V4(..) => TcpSocket::new_v4()?,
        SocketAddr::V6(..) => TcpSocket::new_v6()?,
    };

    // Any traffic to localhost should not be protected
    // This is a workaround for VPNService
    #[cfg(target_os = "android")]
    if !saddr.ip().is_loopback() {
        use std::time::Duration;
        use tokio::time;

        if let Some(ref path) = config.vpn_protect_path {
            // RPC calls to `VpnService.protect()`
            // Timeout in 3 seconds like shadowsocks-libev
            match time::timeout(Duration::from_secs(3), protect(path, socket.as_raw_fd())).await {
                Ok(Ok(..)) => {}
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(Error::new(ErrorKind::TimedOut, "protect() timeout")),
            }
        }
    }

    // Set SO_MARK for mark-based routing on Linux (since 2.6.25)
    // NOTE: This will require CAP_NET_ADMIN capability (root in most cases)
    #[cfg(any(target_os = "linux", target_os = "android"))]
    if let Some(mark) = config.fwmark {
        let ret = unsafe {
            libc::setsockopt(
                socket.as_raw_fd(),
                libc::SOL_SOCKET,
                libc::SO_MARK,
                &mark as *const _ as *const _,
                mem::size_of_val(&mark) as libc::socklen_t,
            )
        };
        if ret != 0 {
            return Err(Error::last_os_error());
        }
    }

    // Set SO_BINDTODEVICE for binding to a specific interface
    #[cfg(any(target_os = "linux", target_os = "android"))]
    if let Some(ref iface) = config.bind_interface {
        use nix::sys::socket::{setsockopt, sockopt::BindToDevice};
        if let Err(err) = setsockopt::<BindToDevice>(socket.as_raw_fd(), BindToDevice, iface) {
            return match err.as_errno() {
                Some(errno) => Err(errno.into()),
                None => Err(Error::new(ErrorKind::Other, err)),
            };
        }
    }

    // Set IP_BOUND_IF for BSD-like
    #[cfg(any(target_os = "macos", target_os = "ios"))]
    if let Some(ref iface) = config.bind_interface {
        const IP_BOUND_IF: libc::c_int = 25; // bsd/netinet/in.h
        const IPV6_BOUND_IF: libc::c_int = 125; // bsd/netinet6/in6.h

        unsafe {
            let mut ciface = [0u8; libc::IFNAMSIZ];
            if iface.len() >= ciface.len() {
                return Err(ErrorKind::InvalidInput.into());
            }

            ptr::copy_nonoverlapping(iface.as_bytes().as_ptr(), ciface.as_mut_ptr(), iface.len());

            let index = libc::if_nametoindex(ciface.as_ptr() as *const libc::c_char);
            if index == 0 {
                return Err(Error::last_os_error());
            }

            let ret = match *saddr {
                SocketAddr::V4(..) => libc::setsockopt(
                    socket.as_raw_fd(),
                    libc::IPPROTO_IP,
                    IP_BOUND_IF,
                    &index as *const _ as *const _,
                    mem::size_of_val(&index) as libc::socklen_t,
                ),
                SocketAddr::V6(..) => libc::setsockopt(
                    socket.as_raw_fd(),
                    libc::IPPROTO_IPV6,
                    IPV6_BOUND_IF,
                    &index as *const _ as *const _,
                    mem::size_of_val(&index) as libc::socklen_t,
                ),
            };

            if ret < 0 {
                return Err(Error::last_os_error());
            }
        }
    }

    // Binds to IP address
    if let Some(ip) = config.bind_local_addr {
        match (ip, saddr.ip()) {
            (IpAddr::V4(..), IpAddr::V4(..)) => {
                socket.bind(SocketAddr::new(ip, 0))?;
            }
            (IpAddr::V6(..), IpAddr::V6(..)) => {
                socket.bind(SocketAddr::new(ip, 0))?;
            }
            _ => {}
        }
    }

    // Set `SO_SNDBUF`
    if let Some(buf_size) = config.tcp.send_buffer_size {
        socket.set_send_buffer_size(buf_size)?;
    }

    // Set `SO_RCVBUF`
    if let Some(buf_size) = config.tcp.recv_buffer_size {
        socket.set_recv_buffer_size(buf_size)?;
    }

    // it's important that the socket is protected before connecting
    let stream = socket.connect(*saddr).await?;

    if config.tcp.nodelay {
        stream.set_nodelay(true)?;
    }

    Ok(stream)
}

/// Create a `UdpSocket` for connecting to `addr`
#[inline(always)]
#[allow(unused_variables)]
pub async fn create_outbound_udp_socket(af: AddrFamily, config: &ConnectOpts) -> io::Result<UdpSocket> {
    let bind_addr = match (af, config.bind_local_addr) {
        (AddrFamily::Ipv4, Some(IpAddr::V4(ip))) => SocketAddr::new(ip.into(), 0),
        (AddrFamily::Ipv6, Some(IpAddr::V6(ip))) => SocketAddr::new(ip.into(), 0),
        (AddrFamily::Ipv4, ..) => SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), 0),
        (AddrFamily::Ipv6, ..) => SocketAddr::new(Ipv6Addr::UNSPECIFIED.into(), 0),
    };

    let socket = UdpSocket::bind(bind_addr).await?;

    // Any traffic except localhost should be protected
    // This is a workaround for VPNService
    #[cfg(target_os = "android")]
    {
        use std::time::Duration;
        use tokio::time;

        if let Some(ref path) = config.vpn_protect_path {
            // RPC calls to `VpnService.protect()`
            // Timeout in 3 seconds like shadowsocks-libev
            match time::timeout(Duration::from_secs(3), protect(path, socket.as_raw_fd())).await {
                Ok(Ok(..)) => {}
                Ok(Err(err)) => return Err(err),
                Err(..) => return Err(Error::new(ErrorKind::TimedOut, "protect() timeout")),
            }
        }
    }

    // Set SO_MARK for mark-based routing on Linux (since 2.6.25)
    // NOTE: This will require CAP_NET_ADMIN capability (root in most cases)
    #[cfg(any(target_os = "linux", target_os = "android"))]
    if let Some(mark) = config.fwmark {
        let ret = unsafe {
            libc::setsockopt(
                socket.as_raw_fd(),
                libc::SOL_SOCKET,
                libc::SO_MARK,
                &mark as *const _ as *const _,
                mem::size_of_val(&mark) as libc::socklen_t,
            )
        };
        if ret != 0 {
            return Err(Error::last_os_error());
        }
    }

    // Set SO_BINDTODEVICE for binding to a specific interface
    #[cfg(any(target_os = "linux", target_os = "android"))]
    if let Some(ref iface) = config.bind_interface {
        use nix::sys::socket::{setsockopt, sockopt::BindToDevice};
        if let Err(err) = setsockopt::<BindToDevice>(socket.as_raw_fd(), BindToDevice, iface) {
            return match err.as_errno() {
                Some(errno) => Err(errno.into()),
                None => Err(Error::new(ErrorKind::Other, err)),
            };
        }
    }

    // Set IP_BOUND_IF for BSD-like
    #[cfg(any(target_os = "macos", target_os = "ios"))]
    if let Some(ref iface) = config.bind_interface {
        const IP_BOUND_IF: libc::c_int = 25; // bsd/netinet/in.h
        const IPV6_BOUND_IF: libc::c_int = 125; // bsd/netinet6/in6.h

        unsafe {
            let mut ciface = [0u8; libc::IFNAMSIZ];
            if iface.len() >= ciface.len() {
                return Err(ErrorKind::InvalidInput.into());
            }

            ptr::copy_nonoverlapping(iface.as_bytes().as_ptr(), ciface.as_mut_ptr(), iface.len());

            let index = libc::if_nametoindex(ciface.as_ptr() as *const libc::c_char);
            if index == 0 {
                return Err(Error::last_os_error());
            }

            let ret = match bind_addr {
                SocketAddr::V4(..) => libc::setsockopt(
                    socket.as_raw_fd(),
                    libc::IPPROTO_IP,
                    IP_BOUND_IF,
                    &index as *const _ as *const _,
                    mem::size_of_val(&index) as libc::socklen_t,
                ),
                SocketAddr::V6(..) => libc::setsockopt(
                    socket.as_raw_fd(),
                    libc::IPPROTO_IPV6,
                    IPV6_BOUND_IF,
                    &index as *const _ as *const _,
                    mem::size_of_val(&index) as libc::socklen_t,
                ),
            };

            if ret < 0 {
                return Err(Error::last_os_error());
            }
        }
    }

    Ok(socket)
}

/// Create a `UdpSocket` binded to `addr`
#[inline(always)]
pub async fn create_inbound_udp_socket(addr: &SocketAddr) -> io::Result<UdpSocket> {
    let set_dual_stack = if let SocketAddr::V6(ref v6) = *addr {
        v6.ip().is_unspecified()
    } else {
        false
    };

    if !set_dual_stack {
        UdpSocket::bind(addr).await
    } else {
        let socket = Socket::new(Domain::for_address(*addr), Type::DGRAM, Some(Protocol::UDP))?;

        if let Err(err) = socket.set_only_v6(false) {
            warn!("failed to set IPV6_V6ONLY: false for listener, error: {}", err);

            // This is not a fatal error, just warn and skip
        }

        let saddr = SockAddr::from(*addr);

        match socket.bind(&saddr) {
            Ok(..) => {}
            Err(ref err) if err.kind() == ErrorKind::AddrInUse => {
                // This is probably 0.0.0.0 with the same port has already been occupied
                debug!(
                    "0.0.0.0:{} may have already been occupied, retry with IPV6_V6ONLY",
                    addr.port()
                );

                if let Err(err) = socket.set_only_v6(true) {
                    warn!("failed to set IPV6_V6ONLY: true for listener, error: {}", err);

                    // This is not a fatal error, just warn and skip
                }
                socket.bind(&saddr)?;
            }
            Err(err) => return Err(err),
        }

        // UdpSocket::from_std requires socket to be non-blocked
        socket.set_nonblocking(true)?;
        UdpSocket::from_std(socket.into())
    }
}
