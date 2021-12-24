//! Asynchronous DNS resolver

use cfg_if::cfg_if;
use log::trace;
use trust_dns_resolver::{
    config::{LookupIpStrategy, ResolverConfig, ResolverOpts},
    error::ResolveResult,
    TokioAsyncResolver,
};

/// Create a `trust-dns` asynchronous DNS resolver
pub async fn create_resolver(dns: Option<ResolverConfig>, ipv6_first: bool) -> ResolveResult<TokioAsyncResolver> {
    let mut resolver_opts = ResolverOpts::default();

    if ipv6_first {
        resolver_opts.ip_strategy = LookupIpStrategy::Ipv6thenIpv4;
    }

    // Customized dns resolution
    match dns {
        Some(conf) => {
            trace!(
                "initializing DNS resolver with config {:?} opts {:?}",
                conf,
                resolver_opts
            );
            TokioAsyncResolver::tokio(conf, resolver_opts)
        }

        // To make this independent, if targeting macOS, BSD, Linux, or Windows, we can use the system's configuration
        // Android doesn't have /etc/resolv.conf.
        None => {
            cfg_if! {
                if #[cfg(any(all(unix, not(target_os = "android")), windows))] {
                    use trust_dns_resolver::{name_server::TokioHandle, system_conf::read_system_conf};

                    // use the system resolver configuration
                    let (config, mut opts) = match read_system_conf() {
                        Ok(o) => o,
                        Err(err) => {
                            use log::error;

                            error!("failed to initialize DNS resolver with system-config, error: {}", err);

                            // From::from is required because on error type is different on Windows
                            #[allow(clippy::useless_conversion)]
                            return Err(From::from(err));
                        }
                    };

                    // NOTE: timeout will be set by config (for example, /etc/resolv.conf on UNIX-like system)
                    //
                    // Only ip_strategy should be changed
                    if ipv6_first {
                        opts.ip_strategy = LookupIpStrategy::Ipv6thenIpv4;
                    }

                    trace!(
                        "initializing DNS resolver with system-config {:?} opts {:?}",
                        config,
                        opts
                    );

                    TokioAsyncResolver::new(config, opts, TokioHandle)
                } else {
                    use trust_dns_resolver::error::ResolveError;

                    Err(ResolveError::from("current platform doesn't support trust-dns resolver with system configured".to_owned()))
                }
            }
        }
    }
}
