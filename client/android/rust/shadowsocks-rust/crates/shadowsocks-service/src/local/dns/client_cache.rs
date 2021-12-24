//! DNS Client cache

#[cfg(unix)]
use std::path::{Path, PathBuf};
use std::{
    collections::{hash_map::Entry, HashMap, VecDeque},
    future::Future,
    io,
    net::SocketAddr,
    time::Duration,
};

use log::trace;
use shadowsocks::{config::ServerConfig, net::ConnectOpts, relay::socks5::Address};
use tokio::sync::Mutex;
use trust_dns_resolver::proto::{error::ProtoError, op::Message};

use crate::local::context::ServiceContext;

use super::upstream::DnsClient;

#[derive(Clone, Debug, Hash, Eq, PartialEq, PartialOrd, Ord)]
enum DnsClientKey {
    TcpLocal(SocketAddr),
    UdpLocal(SocketAddr),
    #[cfg(unix)]
    UnixStream(PathBuf),
    TcpRemote(Address),
    UdpRemote(Address),
}

pub struct DnsClientCache {
    cache: Mutex<HashMap<DnsClientKey, VecDeque<DnsClient>>>,
    timeout: Duration,
    retry_count: usize,
    max_client_per_addr: usize,
}

impl DnsClientCache {
    pub fn new(max_client_per_addr: usize) -> DnsClientCache {
        DnsClientCache {
            cache: Mutex::new(HashMap::new()),
            timeout: Duration::from_secs(5),
            retry_count: 1,
            max_client_per_addr,
        }
    }

    pub async fn lookup_tcp_local(
        &self,
        ns: SocketAddr,
        msg: Message,
        connect_opts: &ConnectOpts,
    ) -> Result<Message, ProtoError> {
        let mut last_err = None;

        for _ in 0..self.retry_count {
            let key = DnsClientKey::TcpLocal(ns);
            let mut client = match self
                .get_client_or_create(&key, async { DnsClient::connect_tcp_local(ns, connect_opts).await })
                .await
            {
                Ok(client) => client,
                Err(err) => {
                    last_err = Some(From::from(err));
                    continue;
                }
            };

            let res = match client.lookup_timeout(msg.clone(), self.timeout).await {
                Ok(msg) => msg,
                Err(error) => {
                    last_err = Some(error);
                    continue;
                }
            };

            self.save_client(key, client).await;

            return Ok(res);
        }

        Err(last_err.unwrap())
    }

    pub async fn lookup_udp_local(
        &self,
        ns: SocketAddr,
        msg: Message,
        connect_opts: &ConnectOpts,
    ) -> Result<Message, ProtoError> {
        let mut last_err = None;

        for _ in 0..self.retry_count {
            let key = DnsClientKey::UdpLocal(ns);
            let mut client = match self
                .get_client_or_create(&key, async { DnsClient::connect_udp_local(ns, connect_opts).await })
                .await
            {
                Ok(client) => client,
                Err(err) => {
                    last_err = Some(From::from(err));
                    continue;
                }
            };

            let res = match client.lookup_timeout(msg.clone(), self.timeout).await {
                Ok(msg) => msg,
                Err(error) => {
                    last_err = Some(error);
                    continue;
                }
            };

            self.save_client(key, client).await;

            return Ok(res);
        }

        Err(last_err.unwrap())
    }

    #[cfg(unix)]
    pub async fn lookup_unix_stream<P: AsRef<Path>>(&self, ns: &P, msg: Message) -> Result<Message, ProtoError> {
        let mut last_err = None;

        let key = DnsClientKey::UnixStream(ns.as_ref().to_path_buf());
        for _ in 0..self.retry_count {
            let mut client = match self
                .get_client_or_create(&key, async { DnsClient::connect_unix_stream(ns).await })
                .await
            {
                Ok(client) => client,
                Err(err) => {
                    last_err = Some(From::from(err));
                    continue;
                }
            };

            let res = match client.lookup_timeout(msg.clone(), self.timeout).await {
                Ok(msg) => msg,
                Err(error) => {
                    last_err = Some(error);
                    continue;
                }
            };

            self.save_client(key, client).await;

            return Ok(res);
        }

        Err(last_err.unwrap())
    }

    pub async fn lookup_tcp_remote(
        &self,
        context: &ServiceContext,
        svr_cfg: &ServerConfig,
        ns: &Address,
        msg: Message,
    ) -> Result<Message, ProtoError> {
        let mut last_err = None;

        let key = DnsClientKey::UdpRemote(ns.clone());
        for _ in 0..self.retry_count {
            let mut client = match self
                .get_client_or_create(&key, async {
                    DnsClient::connect_tcp_remote(
                        context.context(),
                        svr_cfg,
                        ns,
                        context.connect_opts_ref(),
                        context.flow_stat(),
                    )
                    .await
                })
                .await
            {
                Ok(client) => client,
                Err(err) => {
                    last_err = Some(From::from(err));
                    continue;
                }
            };

            let res = match client.lookup_timeout(msg.clone(), self.timeout).await {
                Ok(msg) => msg,
                Err(error) => {
                    last_err = Some(error);
                    continue;
                }
            };

            self.save_client(key, client).await;

            return Ok(res);
        }

        Err(last_err.unwrap())
    }

    pub async fn lookup_udp_remote(
        &self,
        context: &ServiceContext,
        svr_cfg: &ServerConfig,
        ns: &Address,
        msg: Message,
    ) -> Result<Message, ProtoError> {
        let mut last_err = None;

        let key = DnsClientKey::TcpRemote(ns.clone());
        for _ in 0..self.retry_count {
            let mut client = match self
                .get_client_or_create(&key, async {
                    DnsClient::connect_udp_remote(
                        context.context(),
                        svr_cfg,
                        ns.clone(),
                        context.connect_opts_ref(),
                        context.flow_stat(),
                    )
                    .await
                })
                .await
            {
                Ok(client) => client,
                Err(err) => {
                    last_err = Some(From::from(err));
                    continue;
                }
            };

            let res = match client.lookup_timeout(msg.clone(), self.timeout).await {
                Ok(msg) => msg,
                Err(error) => {
                    last_err = Some(error);
                    continue;
                }
            };

            self.save_client(key, client).await;

            return Ok(res);
        }

        Err(last_err.unwrap())
    }

    async fn get_client_or_create<C>(&self, key: &DnsClientKey, create_fn: C) -> io::Result<DnsClient>
    where
        C: Future<Output = io::Result<DnsClient>>,
    {
        // Check if there already is a cached client
        if let Some(q) = self.cache.lock().await.get_mut(key) {
            if let Some(c) = q.pop_front() {
                trace!("take cached DNS client for {:?}", key);
                return Ok(c);
            }
        }

        trace!("creating connection to DNS server {:?}", key);

        // Create one
        create_fn.await
    }

    async fn save_client(&self, key: DnsClientKey, client: DnsClient) {
        match self.cache.lock().await.entry(key) {
            Entry::Occupied(occ) => {
                let q = occ.into_mut();
                q.push_back(client);
                if q.len() > self.max_client_per_addr {
                    q.pop_front();
                }
            }
            Entry::Vacant(vac) => {
                let mut q = VecDeque::with_capacity(5);
                q.push_back(client);
                vac.insert(q);
            }
        }
    }
}
