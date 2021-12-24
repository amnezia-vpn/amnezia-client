//! Replacement of service's DNS resolver

use std::{
    io::{self, ErrorKind},
    net::SocketAddr,
};

use async_trait::async_trait;
use futures::future;
use log::{debug, trace};
use shadowsocks::{config::Mode, dns_resolver::DnsResolve, net::ConnectOpts};
use trust_dns_resolver::proto::{
    op::{Message, Query},
    rr::{DNSClass, Name, RData, RecordType},
};

use super::{client_cache::DnsClientCache, config::NameServerAddr};

pub struct DnsResolver {
    ns: NameServerAddr,
    client_cache: DnsClientCache,
    mode: Mode,
    ipv6_first: bool,
    connect_opts: ConnectOpts,
    attempts: usize,
}

impl DnsResolver {
    pub fn new(ns: NameServerAddr) -> DnsResolver {
        DnsResolver {
            ns,
            client_cache: DnsClientCache::new(5),
            mode: Mode::UdpOnly,
            ipv6_first: false,
            connect_opts: ConnectOpts::default(),
            attempts: 2,
        }
    }

    pub fn set_mode(&mut self, mode: Mode) {
        self.mode = mode;
    }

    pub fn set_ipv6_first(&mut self, ipv6_first: bool) {
        self.ipv6_first = ipv6_first;
    }

    pub fn set_connect_opts(&mut self, connect_opts: ConnectOpts) {
        self.connect_opts = connect_opts;
    }

    async fn lookup(&self, msg: Message) -> io::Result<Message> {
        let mut last_err = io::Error::new(ErrorKind::InvalidData, "resolve empty");

        for _ in 0..self.attempts {
            match self.lookup_inner(msg.clone()).await {
                Ok(m) => return Ok(m),
                Err(err) => last_err = err,
            }
        }

        Err(last_err)
    }

    async fn lookup_inner(&self, msg: Message) -> io::Result<Message> {
        match self.ns {
            NameServerAddr::SocketAddr(ns) => {
                let mut last_err = io::Error::new(ErrorKind::InvalidData, "resolve empty");

                // Query UDP then TCP

                if self.mode.enable_udp() {
                    match self
                        .client_cache
                        .lookup_udp_local(ns, msg.clone(), &self.connect_opts)
                        .await
                    {
                        Ok(msg) => return Ok(msg),
                        Err(err) => {
                            last_err = err.into();
                        }
                    }
                }

                if self.mode.enable_tcp() {
                    match self.client_cache.lookup_tcp_local(ns, msg, &self.connect_opts).await {
                        Ok(msg) => return Ok(msg),
                        Err(err) => {
                            last_err = err.into();
                        }
                    }
                }

                Err(last_err)
            }

            #[cfg(unix)]
            NameServerAddr::UnixSocketAddr(ref path) => self
                .client_cache
                .lookup_unix_stream(path, msg)
                .await
                .map_err(From::from),
        }
    }
}

#[async_trait]
impl DnsResolve for DnsResolver {
    async fn resolve(&self, host: &str, port: u16) -> io::Result<Vec<SocketAddr>> {
        let mut name = Name::from_utf8(host)?;
        name.set_fqdn(true);

        let mut queryv4 = Query::new();
        queryv4.set_query_class(DNSClass::IN);
        queryv4.set_name(name);

        let mut queryv6 = queryv4.clone();
        queryv4.set_query_type(RecordType::A);
        queryv6.set_query_type(RecordType::AAAA);

        let mut msgv4 = Message::new();
        msgv4.set_recursion_desired(true);
        msgv4.add_query(queryv4);

        let mut msgv6 = Message::new();
        msgv6.set_recursion_desired(true);
        msgv6.add_query(queryv6);

        let (res_v4, res_v6) = future::join(self.lookup(msgv4), self.lookup(msgv6)).await;

        if res_v4.is_err() && res_v6.is_err() {
            return if self.ipv6_first {
                Err(res_v6.unwrap_err())
            } else {
                Err(res_v4.unwrap_err())
            };
        }

        let mut vaddr = Vec::new();

        if self.ipv6_first {
            match res_v6 {
                Ok(res) => {
                    for record in res.answers() {
                        match *record.rdata() {
                            RData::A(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            RData::AAAA(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            ref rdata => {
                                trace!("skipped rdata {:?}", rdata);
                            }
                        }
                    }
                }
                Err(err) => {
                    debug!("failed to resolve AAAA records, error: {}", err);
                }
            }

            match res_v4 {
                Ok(res) => {
                    for record in res.answers() {
                        match *record.rdata() {
                            RData::A(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            RData::AAAA(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            ref rdata => {
                                trace!("skipped rdata {:?}", rdata);
                            }
                        }
                    }
                }
                Err(err) => {
                    debug!("failed to resolve A records, error: {}", err);
                }
            }
        } else {
            match res_v4 {
                Ok(res) => {
                    for record in res.answers() {
                        match *record.rdata() {
                            RData::A(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            RData::AAAA(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            ref rdata => {
                                trace!("skipped rdata {:?}", rdata);
                            }
                        }
                    }
                }
                Err(err) => {
                    debug!("failed to resolve A records, error: {}", err);
                }
            }

            match res_v6 {
                Ok(res) => {
                    for record in res.answers() {
                        match *record.rdata() {
                            RData::A(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            RData::AAAA(addr) => vaddr.push(SocketAddr::new(addr.into(), port)),
                            ref rdata => {
                                trace!("skipped rdata {:?}", rdata);
                            }
                        }
                    }
                }
                Err(err) => {
                    debug!("failed to resolve AAAA records, error: {}", err);
                }
            }
        }

        if vaddr.is_empty() {
            let err = io::Error::new(ErrorKind::InvalidData, "resolve empty");
            return Err(err);
        }

        Ok(vaddr)
    }
}
