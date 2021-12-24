//! PacketFilter implementation for *BSD

use std::{
    ffi::CString,
    io::{self, Error, ErrorKind},
    mem,
    net::SocketAddr,
    ptr,
};

use log::trace;
use once_cell::sync::Lazy;
use socket2::{Protocol, SockAddr};

mod ffi {
    use cfg_if::cfg_if;
    use nix::ioctl_readwrite;

    #[repr(C)]
    #[derive(Copy, Clone)]
    pub struct pf_addr {
        pub pfa: pf_addr__bindgen_ty_1,
    }

    #[repr(C)]
    #[derive(Copy, Clone)]
    pub union pf_addr__bindgen_ty_1 {
        pub v4: libc::in_addr,
        pub v6: libc::in6_addr,
        pub addr8: [u8; 16usize],
        pub addr16: [u16; 8usize],
        pub addr32: [u32; 4usize],
        _bindgen_union_align: [u32; 4usize],
    }

    cfg_if! {
        if #[cfg(any(target_os = "macos", target_os = "ios"))] {
            #[repr(C)]
            #[derive(Copy, Clone)]
            pub union pf_state_xport {
                pub port: u16,
                pub call_id: u16,
                pub spi: u32,
            }

            // Apple's XNU customized structure
            //
            // https://github.com/opensource-apple/xnu/blob/master/bsd/net/pfvar.h
            #[repr(C)]
            #[derive(Copy, Clone)]
            pub struct pfioc_natlook {
                pub saddr: pf_addr,
                pub daddr: pf_addr,
                pub rsaddr: pf_addr,
                pub rdaddr: pf_addr,
                pub sxport: pf_state_xport,
                pub dxport: pf_state_xport,
                pub rsxport: pf_state_xport,
                pub rdxport: pf_state_xport,
                pub af: libc::sa_family_t,
                pub proto: u8,
                pub proto_variant: u8,
                pub direction: u8,
            }

            impl pfioc_natlook {
                pub unsafe fn set_sport(&mut self, port: u16) {
                    self.sxport.port = port;
                }

                pub unsafe fn set_dport(&mut self, port: u16) {
                    self.dxport.port = port;
                }

                pub unsafe fn rdport(&self) -> u16 {
                    self.rdxport.port
                }
            }

        } else {
            // FreeBSD's definition, should be the same as all the other platforms
            //
            // https://github.com/freebsd/freebsd/blob/master/sys/net/pfvar.h
            #[repr(C)]
            #[derive(Copy, Clone)]
            pub struct pfioc_natlook {
                pub saddr: pf_addr,
                pub daddr: pf_addr,
                pub rsaddr: pf_addr,
                pub rdaddr: pf_addr,
                pub sport: u16,
                pub dport: u16,
                pub rsport: u16,
                pub rdport: u16,
                pub af: libc::sa_family_t,
                pub proto: u8,
                pub proto_variant: u8,
                pub direction: u8,
            }

            impl pfioc_natlook {
                pub fn set_sport(&mut self, port: u16) {
                    self.sport = port;
                }

                pub fn set_dport(&mut self, port: u16) {
                    self.dport = port;
                }

                pub fn rdport(&self) -> u16 {
                    self.rdport
                }
            }
        }
    }

    // pub const PF_IN: libc::c_int = 1;
    pub const PF_OUT: libc::c_int = 2;

    ioctl_readwrite!(ioc_natlook, 'D', 23, pfioc_natlook);
}

pub struct PacketFilter {
    fd: libc::c_int,
}

impl PacketFilter {
    fn open() -> io::Result<PacketFilter> {
        unsafe {
            let dev_path = CString::new("/dev/pf").expect("CString::new");

            // According to FreeBSD's doc
            // https://www.freebsd.org/cgi/man.cgi?query=pf&sektion=4&apropos=0&manpath=FreeBSD+12.1-RELEASE+and+Ports
            let fd = libc::open(dev_path.as_ptr(), libc::O_RDONLY);
            if fd < 0 {
                let err = Error::last_os_error();
                return Err(err);
            }

            // Set CLOEXEC
            let ret = libc::fcntl(fd, libc::F_SETFD, libc::fcntl(fd, libc::F_GETFD) | libc::FD_CLOEXEC);
            if ret != 0 {
                let err = Error::last_os_error();
                let _ = libc::close(fd);
                return Err(err);
            }

            Ok(PacketFilter { fd })
        }
    }

    pub fn natlook(&self, bind_addr: &SocketAddr, peer_addr: &SocketAddr, proto: Protocol) -> io::Result<SocketAddr> {
        trace!("PF natlook peer: {}, bind: {}", peer_addr, bind_addr);

        unsafe {
            let mut pnl: ffi::pfioc_natlook = mem::zeroed();

            match *bind_addr {
                SocketAddr::V4(ref v4) => {
                    pnl.af = libc::AF_INET as libc::sa_family_t;

                    let sockaddr = SockAddr::from(*v4);
                    let sockaddr = sockaddr.as_ptr() as *const libc::sockaddr_in;

                    let addr: *const libc::in_addr = &((*sockaddr).sin_addr) as *const _;
                    let port: libc::in_port_t = (*sockaddr).sin_port;

                    ptr::copy_nonoverlapping(addr, &mut pnl.daddr.pfa.v4, mem::size_of_val(&pnl.daddr.pfa.v4));
                    pnl.set_dport(port);
                }
                SocketAddr::V6(ref v6) => {
                    pnl.af = libc::AF_INET6 as libc::sa_family_t;

                    let sockaddr = SockAddr::from(*v6);
                    let sockaddr = sockaddr.as_ptr() as *const libc::sockaddr_in6;

                    let addr: *const libc::in6_addr = &((*sockaddr).sin6_addr) as *const _;
                    let port: libc::in_port_t = (*sockaddr).sin6_port;

                    ptr::copy_nonoverlapping(addr, &mut pnl.daddr.pfa.v6, mem::size_of_val(&pnl.daddr.pfa.v6));
                    pnl.set_dport(port);
                }
            }

            match *peer_addr {
                SocketAddr::V4(ref v4) => {
                    if pnl.af != libc::AF_INET as libc::sa_family_t {
                        return Err(Error::new(ErrorKind::InvalidInput, "client addr must be ipv4"));
                    }

                    let sockaddr = SockAddr::from(*v4);
                    let sockaddr = sockaddr.as_ptr() as *const libc::sockaddr_in;

                    let addr: *const libc::in_addr = &((*sockaddr).sin_addr) as *const _;
                    let port: libc::in_port_t = (*sockaddr).sin_port;

                    ptr::copy_nonoverlapping(addr, &mut pnl.saddr.pfa.v4, mem::size_of_val(&pnl.saddr.pfa.v4));
                    pnl.set_sport(port);
                }
                SocketAddr::V6(ref v6) => {
                    if pnl.af != libc::AF_INET6 as libc::sa_family_t {
                        return Err(Error::new(ErrorKind::InvalidInput, "client addr must be ipv6"));
                    }

                    let sockaddr = SockAddr::from(*v6);
                    let sockaddr = sockaddr.as_ptr() as *const libc::sockaddr_in6;

                    let addr: *const libc::in6_addr = &((*sockaddr).sin6_addr) as *const _;
                    let port: libc::in_port_t = (*sockaddr).sin6_port;

                    ptr::copy_nonoverlapping(addr, &mut pnl.saddr.pfa.v6, mem::size_of_val(&pnl.saddr.pfa.v6));
                    pnl.set_sport(port);
                }
            }

            pnl.proto = i32::from(proto) as u8;
            pnl.direction = ffi::PF_OUT as u8;

            if let Err(err) = ffi::ioc_natlook(self.fd, &mut pnl as *mut _) {
                let nerr = match err.as_errno() {
                    Some(errno) => Error::from_raw_os_error(errno as i32),
                    None => Error::new(ErrorKind::Other, "ioctl DIOCNATLOOK"),
                };
                return Err(nerr);
            }

            let (_, dst_addr) = SockAddr::init(|dst_addr, addr_len| {
                if pnl.af == libc::AF_INET as libc::sa_family_t {
                    let dst_addr: &mut libc::sockaddr_in = &mut *(dst_addr as *mut _);
                    dst_addr.sin_family = pnl.af;
                    dst_addr.sin_port = pnl.rdport();
                    ptr::copy_nonoverlapping(
                        &pnl.rdaddr.pfa.v4,
                        &mut dst_addr.sin_addr,
                        mem::size_of_val(&pnl.rdaddr.pfa.v4),
                    );
                    *addr_len = mem::size_of_val(&pnl.rdaddr.pfa.v4) as libc::socklen_t;
                } else if pnl.af == libc::AF_INET6 as libc::sa_family_t {
                    let dst_addr: &mut libc::sockaddr_in6 = &mut *(dst_addr as *mut _);
                    dst_addr.sin6_family = pnl.af;
                    dst_addr.sin6_port = pnl.rdport();
                    ptr::copy_nonoverlapping(
                        &pnl.rdaddr.pfa.v6,
                        &mut dst_addr.sin6_addr,
                        mem::size_of_val(&pnl.rdaddr.pfa.v6),
                    );
                    *addr_len = mem::size_of_val(&pnl.rdaddr.pfa.v6) as libc::socklen_t;
                } else {
                    unreachable!("sockaddr should be either ipv4 or ipv6");
                }

                Ok(())
            })?;

            Ok(dst_addr.as_socket().expect("SocketAddr"))
        }
    }
}

impl Drop for PacketFilter {
    fn drop(&mut self) {
        unsafe {
            libc::close(self.fd);
        }
    }
}

pub static PF: Lazy<PacketFilter> = Lazy::new(|| match PacketFilter::open() {
    Ok(pf) => pf,
    Err(err) if err.kind() == ErrorKind::PermissionDenied => {
        panic!("open /dev/pf permission denied, consider restart with root user");
    }
    Err(err) => {
        panic!("open /dev/pf {}", err);
    }
});
