//! Shadowsocks Local Server Context

use std::sync::Arc;
#[cfg(feature = "local-dns")]
use std::{net::IpAddr, time::Duration};

#[cfg(feature = "local-dns")]
use lru_time_cache::LruCache;
use shadowsocks::{
    config::ServerType,
    context::{Context, SharedContext},
    dns_resolver::DnsResolver,
    net::{AcceptOpts, ConnectOpts},
    relay::Address,
};
#[cfg(feature = "local-dns")]
use tokio::sync::Mutex;

use crate::{acl::AccessControl, net::FlowStat};

/// Local Service Context
pub struct ServiceContext {
    context: SharedContext,
    connect_opts: ConnectOpts,
    accept_opts: AcceptOpts,

    // Access Control
    acl: Option<AccessControl>,

    // Flow statistic report
    flow_stat: Arc<FlowStat>,

    // For DNS relay's ACL domain name reverse lookup -- whether the IP shall be forwarded
    #[cfg(feature = "local-dns")]
    reverse_lookup_cache: Mutex<LruCache<IpAddr, bool>>,
}

impl ServiceContext {
    /// Create a new `ServiceContext`
    pub fn new() -> ServiceContext {
        ServiceContext {
            context: Context::new_shared(ServerType::Local),
            connect_opts: ConnectOpts::default(),
            accept_opts: AcceptOpts::default(),
            acl: None,
            flow_stat: Arc::new(FlowStat::new()),
            #[cfg(feature = "local-dns")]
            reverse_lookup_cache: Mutex::new(LruCache::with_expiry_duration(Duration::from_secs(3 * 24 * 60 * 60))),
        }
    }

    /// Get cloned `shadowsocks` Context
    pub fn context(&self) -> SharedContext {
        self.context.clone()
    }

    /// Get `shadowsocks` Context reference
    pub fn context_ref(&self) -> &Context {
        self.context.as_ref()
    }

    /// Set `ConnectOpts`
    pub fn set_connect_opts(&mut self, connect_opts: ConnectOpts) {
        self.connect_opts = connect_opts;
    }

    /// Get `ConnectOpts` reference
    pub fn connect_opts_ref(&self) -> &ConnectOpts {
        &self.connect_opts
    }

    /// Set `AcceptOpts`
    pub fn set_accept_opts(&mut self, accept_opts: AcceptOpts) {
        self.accept_opts = accept_opts;
    }

    /// Get `AcceptOpts` cloned
    pub fn accept_opts(&self) -> AcceptOpts {
        self.accept_opts.clone()
    }

    /// Set Access Control List
    pub fn set_acl(&mut self, acl: AccessControl) {
        self.acl = Some(acl);
    }

    /// Get Access Control List reference
    pub fn acl(&self) -> Option<&AccessControl> {
        self.acl.as_ref()
    }

    /// Get cloned flow statistic
    pub fn flow_stat(&self) -> Arc<FlowStat> {
        self.flow_stat.clone()
    }

    /// Get flow statistic reference
    pub fn flow_stat_ref(&self) -> &FlowStat {
        self.flow_stat.as_ref()
    }

    /// Set customized DNS resolver
    pub fn set_dns_resolver(&mut self, resolver: Arc<DnsResolver>) {
        let context = Arc::get_mut(&mut self.context).expect("cannot set DNS resolver on a shared context");
        context.set_dns_resolver(resolver)
    }

    /// Get reference of DNS resolver
    pub fn dns_resolver(&self) -> &DnsResolver {
        self.context.dns_resolver()
    }

    /// Check if target should be bypassed
    pub async fn check_target_bypassed(&self, addr: &Address) -> bool {
        match self.acl {
            None => false,
            Some(ref acl) => {
                #[cfg(feature = "local-dns")]
                {
                    if let Address::SocketAddress(ref saddr) = addr {
                        // do the reverse lookup in our local cache
                        let mut reverse_lookup_cache = self.reverse_lookup_cache.lock().await;
                        // if a qname is found
                        if let Some(forward) = reverse_lookup_cache.get(&saddr.ip()) {
                            return !*forward;
                        }
                    }
                }

                acl.check_target_bypassed(&self.context, addr).await
            }
        }
    }

    /// Add a record to the reverse lookup cache
    #[cfg(feature = "local-dns")]
    pub async fn add_to_reverse_lookup_cache(&self, addr: IpAddr, forward: bool) {
        let is_exception = forward
            != match self.acl {
                // Proxy everything by default
                None => true,
                Some(ref a) => a.check_ip_in_proxy_list(&addr),
            };
        let mut reverse_lookup_cache = self.reverse_lookup_cache.lock().await;
        match reverse_lookup_cache.get_mut(&addr) {
            Some(value) => {
                if is_exception {
                    *value = forward;
                } else {
                    // we do not need to remember the entry if it is already matched correctly
                    reverse_lookup_cache.remove(&addr);
                }
            }
            None => {
                if is_exception {
                    reverse_lookup_cache.insert(addr, forward);
                }
            }
        }
    }
}
