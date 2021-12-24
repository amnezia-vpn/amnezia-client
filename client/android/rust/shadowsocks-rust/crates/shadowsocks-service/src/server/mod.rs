//! Shadowsocks server

use std::{io, sync::Arc};

use futures::{future, FutureExt};
use log::{trace, warn};
use shadowsocks::net::{AcceptOpts, ConnectOpts};

use crate::{
    config::{Config, ConfigType},
    dns::build_dns_resolver,
};

pub use self::server::Server;

pub mod context;
pub mod server;
mod tcprelay;
mod udprelay;

/// Starts a shadowsocks server
pub async fn run(config: Config) -> io::Result<()> {
    assert_eq!(config.config_type, ConfigType::Server);
    assert!(config.server.len() > 0);

    trace!("{:?}", config);

    // Warning for Stream Ciphers
    #[cfg(feature = "stream-cipher")]
    for server in config.server.iter() {
        if server.method().is_stream() {
            warn!("stream cipher {} for server {} have inherent weaknesses (see discussion in https://github.com/shadowsocks/shadowsocks-org/issues/36). \
                    DO NOT USE. It will be removed in the future.", server.method(), server.addr());
        }
    }

    #[cfg(unix)]
    if let Some(nofile) = config.nofile {
        use crate::sys::set_nofile;
        if let Err(err) = set_nofile(nofile) {
            warn!("set_nofile {} failed, error: {}", nofile, err);
        }
    }

    let mut servers = Vec::new();

    let mut connect_opts = ConnectOpts {
        #[cfg(any(target_os = "linux", target_os = "android"))]
        fwmark: config.outbound_fwmark,

        #[cfg(target_os = "android")]
        vpn_protect_path: config.outbound_vpn_protect_path,

        bind_local_addr: config.local_addr,

        #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos", target_os = "ios"))]
        bind_interface: config.outbound_bind_interface,

        ..Default::default()
    };

    connect_opts.tcp.send_buffer_size = config.outbound_send_buffer_size;
    connect_opts.tcp.recv_buffer_size = config.outbound_recv_buffer_size;
    connect_opts.tcp.nodelay = config.no_delay;

    let mut accept_opts = AcceptOpts::default();
    accept_opts.tcp.send_buffer_size = config.inbound_send_buffer_size;
    accept_opts.tcp.recv_buffer_size = config.inbound_recv_buffer_size;
    accept_opts.tcp.nodelay = config.no_delay;

    let resolver = match build_dns_resolver(config.dns, config.ipv6_first, &connect_opts).await {
        Some(resolver) => Some(Arc::new(resolver)),
        None => None,
    };

    let acl = config.acl.map(Arc::new);

    for svr_cfg in config.server {
        let mut server = Server::new(svr_cfg);

        if let Some(ref r) = resolver {
            server.set_dns_resolver(r.clone());
        }

        server.set_connect_opts(connect_opts.clone());
        server.set_accept_opts(accept_opts.clone());

        if let Some(c) = config.udp_max_associations {
            server.set_udp_capacity(c);
        }
        if let Some(d) = config.udp_timeout {
            server.set_udp_expiry_duration(d);
        }
        if let Some(ref m) = config.manager {
            server.set_manager_addr(m.addr.clone());
        }

        if let Some(ref acl) = acl {
            server.set_acl(acl.clone());
        }

        servers.push(server);
    }

    let mut vfut = Vec::with_capacity(servers.len());
    for server in servers {
        vfut.push(server.run().boxed());
    }

    let (res, ..) = future::select_all(vfut).await;
    res
}
