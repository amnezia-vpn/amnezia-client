//! A TCP listener for accepting shadowsocks' client connection

use std::{io, net::SocketAddr};

use once_cell::sync::Lazy;
use tokio::{
    io::{AsyncRead, AsyncWrite},
    net::TcpStream,
};

use crate::{
    config::{ServerAddr, ServerConfig},
    context::SharedContext,
    crypto::v1::CipherKind,
    net::{AcceptOpts, TcpListener},
    relay::tcprelay::proxy_stream::server::ProxyServerStream,
};

/// A TCP listener for accepting shadowsocks' client connection
pub struct ProxyListener {
    listener: TcpListener,
    method: CipherKind,
    key: Box<[u8]>,
    context: SharedContext,
}

static DEFAULT_ACCEPT_OPTS: Lazy<AcceptOpts> = Lazy::new(Default::default);

impl ProxyListener {
    /// Create a `ProxyListener` binding to a specific address
    pub async fn bind(context: SharedContext, svr_cfg: &ServerConfig) -> io::Result<ProxyListener> {
        ProxyListener::bind_with_opts(context, svr_cfg, DEFAULT_ACCEPT_OPTS.clone()).await
    }

    /// Create a `ProxyListener` binding to a specific address with opts
    pub async fn bind_with_opts(
        context: SharedContext,
        svr_cfg: &ServerConfig,
        accept_opts: AcceptOpts,
    ) -> io::Result<ProxyListener> {
        let listener = match svr_cfg.external_addr() {
            ServerAddr::SocketAddr(sa) => TcpListener::bind_with_opts(sa, accept_opts).await?,
            ServerAddr::DomainName(domain, port) => {
                lookup_then!(&context, &domain, *port, |addr| {
                    TcpListener::bind_with_opts(&addr, accept_opts.clone()).await
                })?
                .1
            }
        };
        Ok(ProxyListener::from_listener(context, listener, svr_cfg))
    }

    /// Create a `ProxyListener` from a `TcpListener`
    pub fn from_listener(context: SharedContext, listener: TcpListener, svr_cfg: &ServerConfig) -> ProxyListener {
        ProxyListener {
            listener,
            method: svr_cfg.method(),
            key: svr_cfg.key().to_vec().into_boxed_slice(),
            context,
        }
    }

    /// Accepts a shadowsocks' client connection
    #[inline]
    pub async fn accept(&self) -> io::Result<(ProxyServerStream<TcpStream>, SocketAddr)> {
        self.accept_map(|s| s).await
    }

    /// Accepts a shadowsocks' client connection and maps the accepted `TcpStream` to another stream type
    pub async fn accept_map<F, S>(&self, map_fn: F) -> io::Result<(ProxyServerStream<S>, SocketAddr)>
    where
        F: FnOnce(TcpStream) -> S,
        S: AsyncRead + AsyncWrite + Unpin,
    {
        let (stream, peer_addr) = self.listener.accept().await?;
        let stream = map_fn(stream);

        // Create a ProxyServerStream and read the target address from it
        let stream = ProxyServerStream::from_stream(self.context.clone(), stream, self.method, &self.key);

        Ok((stream, peer_addr))
    }

    /// Get local binded address
    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.listener.local_addr()
    }

    /// Get reference to the internal listener
    pub fn get_ref(&self) -> &TcpListener {
        &self.listener
    }

    /// Consumes the `ProxyListener` and return the internal listener
    pub fn into_inner(self) -> TcpListener {
        self.listener
    }
}
