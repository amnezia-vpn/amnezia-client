//! Resolver Alternatives

#[cfg(feature = "trust-dns")]
use std::sync::Arc;
use std::{
    fmt::{self, Debug},
    io::{self, Error, ErrorKind},
    net::SocketAddr,
    time::Instant,
};

#[cfg(feature = "trust-dns")]
use arc_swap::ArcSwap;
use async_trait::async_trait;
#[cfg(feature = "trust-dns")]
use futures::future::{self, AbortHandle};
use log::{error, log_enabled, trace, Level};
use tokio::net::lookup_host;
#[cfg(feature = "trust-dns")]
use trust_dns_resolver::{config::ResolverConfig, TokioAsyncResolver};

/// Abstract DNS resolver
#[async_trait]
pub trait DnsResolve {
    /// Resolves `addr:port` to a list of `SocketAddr`
    async fn resolve(&self, addr: &str, port: u16) -> io::Result<Vec<SocketAddr>>;
}

#[cfg(feature = "trust-dns")]
pub struct TrustDnsSystemResolver {
    resolver: ArcSwap<TokioAsyncResolver>,
    ipv6_first: bool,
}

/// Collections of DNS resolver
pub enum DnsResolver {
    /// System Resolver, which is tokio's builtin resolver
    System,
    /// Trust-DNS's system resolver
    #[cfg(feature = "trust-dns")]
    TrustDnsSystem {
        inner: Arc<TrustDnsSystemResolver>,
        abortable: AbortHandle,
    },
    #[cfg(feature = "trust-dns")]
    /// Trust-DNS resolver
    TrustDns(TokioAsyncResolver),
    /// Customized Resolver
    Custom(Box<dyn DnsResolve + Send + Sync>),
}

impl Default for DnsResolver {
    fn default() -> DnsResolver {
        DnsResolver::system_resolver()
    }
}

impl Debug for DnsResolver {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            DnsResolver::System => f.write_str("System"),
            #[cfg(feature = "trust-dns")]
            DnsResolver::TrustDnsSystem { .. } => f.write_str("TrustDnsSystem(..)"),
            #[cfg(feature = "trust-dns")]
            DnsResolver::TrustDns(..) => f.write_str("TrustDns(..)"),
            DnsResolver::Custom(..) => f.write_str("Custom(..)"),
        }
    }
}

#[cfg(feature = "trust-dns")]
impl Drop for DnsResolver {
    fn drop(&mut self) {
        if let DnsResolver::TrustDnsSystem { ref abortable, .. } = *self {
            abortable.abort();
        }
    }
}

struct EmptyResolveResult;

impl Iterator for EmptyResolveResult {
    type Item = SocketAddr;

    fn next(&mut self) -> Option<SocketAddr> {
        None
    }
}

// Resolved result
enum EitherResolved<A = EmptyResolveResult, B = EmptyResolveResult, C = EmptyResolveResult, D = EmptyResolveResult> {
    Tokio(A),
    TrustDnsSystem(B),
    TrustDns(C),
    Custom(D),
}

impl<A, B, C, D> Iterator for EitherResolved<A, B, C, D>
where
    A: Iterator<Item = SocketAddr>,
    B: Iterator<Item = SocketAddr>,
    C: Iterator<Item = SocketAddr>,
    D: Iterator<Item = SocketAddr>,
{
    type Item = SocketAddr;

    fn next(&mut self) -> Option<SocketAddr> {
        match *self {
            EitherResolved::Tokio(ref mut a) => a.next(),
            EitherResolved::TrustDnsSystem(ref mut b) => b.next(),
            EitherResolved::TrustDns(ref mut c) => c.next(),
            EitherResolved::Custom(ref mut d) => d.next(),
        }
    }
}

#[cfg(all(feature = "trust-dns", unix, not(target_os = "android")))]
async fn trust_dns_notify_update_dns(resolver: Arc<TrustDnsSystemResolver>) -> notify::Result<()> {
    use log::debug;
    use notify::{Event, EventKind, RecommendedWatcher, RecursiveMode, Result as NotifyResult, Watcher};
    use tokio::sync::watch;

    use super::trust_dns_resolver::create_resolver;

    let (tx, mut rx) = watch::channel::<Event>(Event::default());

    let mut watcher: RecommendedWatcher =
        Watcher::new_immediate(move |ev_result: NotifyResult<Event>| match ev_result {
            Ok(ev) => {
                trace!("received event {:?}", ev);

                if let EventKind::Modify(..) = ev.kind {
                    tx.send(ev).expect("watcher.send");
                }
            }
            Err(err) => {
                error!("watching /etc/resolv.conf error: {}", err);
            }
        })?;

    // NOTE: It is an undefined behavior if this file get renamed or removed.
    watcher.watch("/etc/resolv.conf", RecursiveMode::NonRecursive)?;

    while rx.changed().await.is_ok() {
        trace!("received notify /etc/resolv.conf changed");

        let new_resolver = match create_resolver(None, resolver.ipv6_first).await {
            Ok(r) => r,
            Err(err) => {
                error!("failed to reload /etc/resolv.conf, error: {}", err);
                continue;
            }
        };

        debug!("auto-reload /etc/resolv.conf");

        resolver.resolver.store(Arc::new(new_resolver));
    }

    error!("auto-reload /etc/resolv.conf task exited unexpectly");

    Ok(())
}

#[cfg(all(feature = "trust-dns", any(not(unix), target_os = "android")))]
async fn trust_dns_notify_update_dns(resolver: Arc<TrustDnsSystemResolver>) -> notify::Result<()> {
    let _ = resolver.ipv6_first; // use it for supressing warning
    future::pending().await
}

impl DnsResolver {
    /// Use system DNS resolver. Tokio will call `getaddrinfo` in blocking pool.
    pub fn system_resolver() -> DnsResolver {
        DnsResolver::System
    }

    /// Use trust-dns DNS system resolver (with DNS cache)
    ///
    /// On *nix system, it will try to read configurations from `/etc/resolv.conf`.
    #[cfg(feature = "trust-dns")]
    pub async fn trust_dns_system_resolver(ipv6_first: bool) -> io::Result<DnsResolver> {
        use super::trust_dns_resolver::create_resolver;

        let resolver = create_resolver(None, ipv6_first).await?;

        let inner = Arc::new(TrustDnsSystemResolver {
            resolver: ArcSwap::from(Arc::new(resolver)),
            ipv6_first,
        });

        let (notify_task, abortable) = {
            let inner = inner.clone();
            future::abortable(async {
                if let Err(err) = trust_dns_notify_update_dns(inner).await {
                    error!("failed to watch DNS system configuration changes, error: {}", err);
                }
            })
        };
        tokio::spawn(notify_task);

        Ok(DnsResolver::TrustDnsSystem { inner, abortable })
    }

    /// Use trust-dns DNS resolver (with DNS cache)
    #[cfg(feature = "trust-dns")]
    pub async fn trust_dns_resolver(dns: ResolverConfig, ipv6_first: bool) -> io::Result<DnsResolver> {
        use super::trust_dns_resolver::create_resolver;
        Ok(DnsResolver::TrustDns(create_resolver(Some(dns), ipv6_first).await?))
    }

    /// Custom DNS resolver
    pub fn custom_resolver<R>(custom: R) -> DnsResolver
    where
        R: DnsResolve + Send + Sync + 'static,
    {
        DnsResolver::Custom(Box::new(custom) as Box<dyn DnsResolve + Send + Sync>)
    }

    /// Resolve address into `SocketAddr`s
    pub async fn resolve<'a>(&self, addr: &'a str, port: u16) -> io::Result<impl Iterator<Item = SocketAddr> + 'a> {
        struct ResolverLogger<'x, 'y> {
            resolver: &'x DnsResolver,
            addr: &'y str,
            port: u16,
            start_time: Option<Instant>,
        }

        impl<'x, 'y> ResolverLogger<'x, 'y> {
            fn new(resolver: &'x DnsResolver, addr: &'y str, port: u16) -> ResolverLogger<'x, 'y> {
                let start_time = if log_enabled!(Level::Trace) {
                    Some(Instant::now())
                } else {
                    None
                };

                ResolverLogger {
                    resolver,
                    addr,
                    port,
                    start_time,
                }
            }
        }

        impl<'x, 'y> Drop for ResolverLogger<'x, 'y> {
            fn drop(&mut self) {
                match self.start_time {
                    Some(start_time) => {
                        let end_time = Instant::now();
                        let elapsed = end_time - start_time;

                        match *self.resolver {
                            DnsResolver::System => {
                                trace!(
                                    "DNS resolved {}:{} with tokio {}s",
                                    self.addr,
                                    self.port,
                                    elapsed.as_secs_f32()
                                );
                            }
                            #[cfg(feature = "trust-dns")]
                            DnsResolver::TrustDnsSystem { .. } | DnsResolver::TrustDns(..) => {
                                trace!(
                                    "DNS resolved {}:{} with trust-dns {}s",
                                    self.addr,
                                    self.port,
                                    elapsed.as_secs_f32()
                                );
                            }
                            DnsResolver::Custom(..) => {
                                trace!(
                                    "DNS resolved {}:{} with customized {}s",
                                    self.addr,
                                    self.port,
                                    elapsed.as_secs_f32()
                                );
                            }
                        }
                    }
                    None => match *self.resolver {
                        DnsResolver::System => {
                            trace!("DNS resolved {}:{} with tokio", self.addr, self.port);
                        }
                        #[cfg(feature = "trust-dns")]
                        DnsResolver::TrustDnsSystem { .. } | DnsResolver::TrustDns(..) => {
                            trace!("DNS resolved {}:{} with trust-dns", self.addr, self.port);
                        }
                        DnsResolver::Custom(..) => {
                            trace!("DNS resolved {}:{} with customized", self.addr, self.port);
                        }
                    },
                }
            }
        }

        let _log_guard = ResolverLogger::new(self, addr, port);

        match *self {
            DnsResolver::System => match lookup_host((addr, port)).await {
                Ok(v) => Ok(EitherResolved::Tokio(v)),
                Err(err) => {
                    let err = Error::new(
                        ErrorKind::Other,
                        format!("dns resolve {}:{} error: {}", addr, port, err),
                    );
                    Err(err)
                }
            },
            #[cfg(feature = "trust-dns")]
            DnsResolver::TrustDnsSystem { ref inner, .. } => match inner.resolver.load().lookup_ip(addr).await {
                Ok(lookup_result) => Ok(EitherResolved::TrustDnsSystem(
                    lookup_result.into_iter().map(move |ip| SocketAddr::new(ip, port)),
                )),
                Err(err) => {
                    let err = Error::new(
                        ErrorKind::Other,
                        format!("dns resolve {}:{} error: {}", addr, port, err),
                    );
                    Err(err)
                }
            },
            #[cfg(feature = "trust-dns")]
            DnsResolver::TrustDns(ref resolver) => match resolver.lookup_ip(addr).await {
                Ok(lookup_result) => Ok(EitherResolved::TrustDns(
                    lookup_result.into_iter().map(move |ip| SocketAddr::new(ip, port)),
                )),
                Err(err) => {
                    let err = Error::new(
                        ErrorKind::Other,
                        format!("dns resolve {}:{} error: {}", addr, port, err),
                    );
                    Err(err)
                }
            },
            DnsResolver::Custom(ref resolver) => match resolver.resolve(addr, port).await {
                Ok(v) => Ok(EitherResolved::Custom(v.into_iter())),
                Err(err) => {
                    let err = Error::new(
                        ErrorKind::Other,
                        format!("dns resolve {}:{} error: {}", addr, port, err),
                    );
                    Err(err)
                }
            },
        }
    }

    /// Check if currently using system resolver
    pub fn is_system_resolver(&self) -> bool {
        matches!(*self, DnsResolver::System)
    }
}
