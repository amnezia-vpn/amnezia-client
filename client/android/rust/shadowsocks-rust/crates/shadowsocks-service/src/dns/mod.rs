//! DNS resolvers

use shadowsocks::{dns_resolver::DnsResolver, net::ConnectOpts};

use crate::config::DnsConfig;

#[allow(unused_variables, dead_code)]
pub async fn build_dns_resolver(dns: DnsConfig, ipv6_first: bool, connect_opts: &ConnectOpts) -> Option<DnsResolver> {
    match dns {
        DnsConfig::System => {
            #[cfg(feature = "trust-dns")]
            if crate::hint_support_default_system_resolver() {
                use log::warn;

                return match DnsResolver::trust_dns_system_resolver(ipv6_first).await {
                    Ok(r) => Some(r),
                    Err(err) => {
                        warn!(
                            "initialize trust-dns DNS system resolver failed, fallback to default system resolver, error: {}",
                            err
                        );
                        None
                    }
                };
            }

            None
        }
        #[cfg(feature = "trust-dns")]
        DnsConfig::TrustDns(dns) => match DnsResolver::trust_dns_resolver(dns, ipv6_first).await {
            Ok(r) => Some(r),
            Err(err) => {
                use log::warn;

                warn!(
                    "initialize trust-dns DNS resolver failed, fallback to default system resolver, error: {}",
                    err
                );
                None
            }
        },
        #[cfg(feature = "local-dns")]
        DnsConfig::LocalDns(ns) => {
            use crate::local::dns::dns_resolver::DnsResolver as LocalDnsResolver;
            use log::trace;
            use shadowsocks::config::Mode;

            trace!("initializing direct DNS resolver for {}", ns);

            let mut resolver = LocalDnsResolver::new(ns);
            resolver.set_mode(Mode::TcpAndUdp);
            resolver.set_ipv6_first(ipv6_first);
            resolver.set_connect_opts(connect_opts.clone());

            Some(DnsResolver::custom_resolver(resolver))
        }
    }
}
