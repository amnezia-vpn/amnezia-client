//! Shadowsocks Local HTTP(S) Server

use std::{
    convert::Infallible,
    io::{self, ErrorKind},
    sync::Arc,
};

use hyper::{
    server::conn::AddrStream,
    service::{make_service_fn, service_fn},
    Body,
    Client,
    Request,
    Server,
};
use log::{error, info};
use shadowsocks::{config::ServerAddr, lookup_then};

use crate::local::{context::ServiceContext, loadbalancing::PingBalancer};

use super::{client_cache::ProxyClientCache, connector::BypassConnector, dispatcher::HttpDispatcher};

/// HTTP Local server
pub struct Http {
    context: Arc<ServiceContext>,
    proxy_client_cache: Arc<ProxyClientCache>,
}

impl Http {
    /// Create a new HTTP Local server
    pub fn new() -> Http {
        let context = ServiceContext::new();
        Http::with_context(Arc::new(context))
    }

    /// Create with an existed context
    pub fn with_context(context: Arc<ServiceContext>) -> Http {
        let proxy_client_cache = Arc::new(ProxyClientCache::new(context.clone()));
        Http {
            context,
            proxy_client_cache,
        }
    }

    /// Run server
    pub async fn run(self, client_config: &ServerAddr, balancer: PingBalancer) -> io::Result<()> {
        let bypass_client = Client::builder().build::<_, Body>(BypassConnector::new(self.context.clone()));

        let context = self.context.clone();
        let proxy_client_cache = self.proxy_client_cache.clone();
        let make_service = make_service_fn(|socket: &AddrStream| {
            let client_addr = socket.remote_addr();
            let balancer = balancer.clone();
            let bypass_client = bypass_client.clone();
            let context = context.clone();
            let proxy_client_cache = proxy_client_cache.clone();

            async move {
                Ok::<_, Infallible>(service_fn(move |req: Request<Body>| {
                    HttpDispatcher::new(
                        context.clone(),
                        req,
                        balancer.clone(),
                        client_addr,
                        bypass_client.clone(),
                        proxy_client_cache.clone(),
                    )
                    .dispatch()
                }))
            }
        });

        let bind_result = match *client_config {
            ServerAddr::SocketAddr(sa) => Server::try_bind(&sa),
            ServerAddr::DomainName(ref dname, port) => lookup_then!(self.context.context_ref(), dname, port, |addr| {
                Server::try_bind(&addr)
            })
            .map(|(_, b)| b),
        };

        // HTTP Proxy protocol only defined in HTTP 1.x
        let server = match bind_result {
            Ok(builder) => builder
                .http1_only(true)
                .tcp_sleep_on_accept_errors(true)
                .serve(make_service),
            Err(err) => {
                error!("hyper server bind error: {}", err);
                let err = io::Error::new(ErrorKind::InvalidInput, err);
                return Err(err);
            }
        };

        info!("shadowsocks HTTP listening on {}", server.local_addr());

        if let Err(err) = server.await {
            use std::io::Error;

            error!("hyper server exited with error: {}", err);
            return Err(Error::new(ErrorKind::Other, err));
        }

        Ok(())
    }
}
