//! HTTP Service Dispatcher

use std::{io, net::SocketAddr, str::FromStr, sync::Arc};

use http::uri::{Authority, Scheme};
use hyper::{header::HeaderValue, upgrade, Body, HeaderMap, Method, Request, Response, StatusCode, Uri, Version};
use log::{debug, error, trace};
use shadowsocks::relay::socks5::Address;

use crate::local::{
    context::ServiceContext,
    loadbalancing::PingBalancer,
    net::AutoProxyClientStream,
    utils::establish_tcp_tunnel,
};

use super::{
    client_cache::ProxyClientCache,
    http_client::BypassHttpClient,
    utils::{authority_addr, host_addr},
};

pub struct HttpDispatcher {
    context: Arc<ServiceContext>,
    req: Request<Body>,
    balancer: PingBalancer,
    client_addr: SocketAddr,
    bypass_client: BypassHttpClient,
    proxy_client_cache: Arc<ProxyClientCache>,
}

impl HttpDispatcher {
    pub fn new(
        context: Arc<ServiceContext>,
        req: Request<Body>,
        balancer: PingBalancer,
        client_addr: SocketAddr,
        bypass_client: BypassHttpClient,
        proxy_client_cache: Arc<ProxyClientCache>,
    ) -> HttpDispatcher {
        HttpDispatcher {
            context,
            req,
            balancer,
            client_addr,
            bypass_client,
            proxy_client_cache,
        }
    }

    pub async fn dispatch(mut self) -> io::Result<Response<Body>> {
        trace!("request {} {:?}", self.client_addr, self.req);

        // Parse URI
        //
        // Proxy request URI must contains a host
        let host = match host_addr(self.req.uri()) {
            None => {
                if self.req.uri().authority().is_some() {
                    // URI has authority but invalid
                    error!(
                        "HTTP {} URI {} doesn't have a valid host",
                        self.req.method(),
                        self.req.uri()
                    );
                    return make_bad_request();
                } else {
                    trace!(
                        "HTTP {} URI {} doesn't have a valid host",
                        self.req.method(),
                        self.req.uri()
                    );
                }

                match get_addr_from_header(&mut self.req) {
                    Ok(h) => h,
                    Err(()) => return make_bad_request(),
                }
            }
            Some(h) => h,
        };

        if Method::CONNECT == self.req.method() {
            // Establish a TCP tunnel
            // https://tools.ietf.org/html/draft-luotonen-web-proxy-tunneling-01

            debug!("HTTP CONNECT {}", host);

            // Connect to Shadowsocks' remote
            //
            // FIXME: What STATUS should I return for connection error?
            let server = self.balancer.best_tcp_server();
            let stream = AutoProxyClientStream::connect(self.context, server.as_ref(), &host).await?;

            debug!("CONNECT relay connected {} <-> {}", self.client_addr, host);

            // Upgrade to a TCP tunnel
            //
            // Note: only after client received an empty body with STATUS_OK can the
            // connection be upgraded, so we can't return a response inside
            // `on_upgrade` future.
            let req = self.req;
            let client_addr = self.client_addr;
            tokio::spawn(async move {
                match upgrade::on(req).await {
                    Ok(upgraded) => {
                        trace!("CONNECT tunnel upgrade success, {} <-> {}", client_addr, host);

                        use tokio::io::split;

                        let (mut plain_reader, mut plain_writer) = split(upgraded);
                        let (mut shadow_reader, mut shadow_writer) = stream.into_split();

                        let _ = establish_tcp_tunnel(
                            server.server_config(),
                            &mut plain_reader,
                            &mut plain_writer,
                            &mut shadow_reader,
                            &mut shadow_writer,
                            client_addr,
                            &host,
                        )
                        .await;
                    }
                    Err(e) => {
                        error!(
                            "failed to upgrade TCP tunnel {} <-> {}, error: {}",
                            client_addr, host, e
                        );
                    }
                }
            });

            // Connection established
            let resp = Response::builder().body(Body::empty()).unwrap();

            Ok(resp)
        } else {
            let method = self.req.method().clone();
            let version = self.req.version();

            debug!("HTTP {} {} {:?}", method, host, version);

            // Check if client wants us to keep long connection
            let conn_keep_alive = check_keep_alive(version, self.req.headers(), true);

            // Remove non-forwardable headers
            clear_hop_headers(self.req.headers_mut());

            // Set keep-alive for connection with remote
            set_conn_keep_alive(version, self.req.headers_mut(), conn_keep_alive);

            let mut res = if self.context.check_target_bypassed(&host).await {
                trace!("bypassed {} -> {} {:?}", self.client_addr, host, self.req);

                // Keep connections in a global client instance
                match self.bypass_client.request(self.req).await {
                    Ok(res) => res,
                    Err(err) => {
                        error!(
                            "HTTP {} {} <-> {} relay failed, error: {}",
                            method, self.client_addr, host, err
                        );

                        let mut resp = Response::new(Body::from(format!("relay failed to {}", host)));
                        *resp.status_mut() = StatusCode::INTERNAL_SERVER_ERROR;

                        return Ok(resp);
                    }
                }
            } else {
                trace!("proxied {} -> {} {:?}", self.client_addr, host, self.req);

                // Keep connections for clients in ServerScore::client
                //
                // client instance is kept for Keep-Alive connections
                let server = self.balancer.best_tcp_server();
                let client = self.proxy_client_cache.get_connected(&server).await;

                match client.request(self.req).await {
                    Ok(res) => res,
                    Err(err) => {
                        error!(
                            "HTTP {} {} <-> {} relay failed, error: {}",
                            method, self.client_addr, host, err
                        );

                        let mut resp = Response::new(Body::from(format!("relay failed to {}", host)));
                        *resp.status_mut() = StatusCode::INTERNAL_SERVER_ERROR;

                        return Ok(resp);
                    }
                }
            };

            trace!("received {} <- {} {:?}", self.client_addr, host, res);

            let res_keep_alive = conn_keep_alive && check_keep_alive(res.version(), res.headers(), false);

            // Clear unforwardable headers
            clear_hop_headers(res.headers_mut());

            if res.version() != version {
                // Reset version to matches req's version
                trace!("response version {:?} => {:?}", res.version(), version);
                *res.version_mut() = version;
            }

            // Set Connection header
            set_conn_keep_alive(res.version(), res.headers_mut(), res_keep_alive);

            trace!("response {} <- {} {:?}", self.client_addr, host, res);

            debug!("HTTP {} relay {} <-> {} finished", method, self.client_addr, host);

            Ok(res)
        }
    }
}

fn make_bad_request() -> io::Result<Response<Body>> {
    let mut resp = Response::new(Body::empty());
    *resp.status_mut() = StatusCode::BAD_REQUEST;
    Ok(resp)
}

fn check_keep_alive(version: Version, headers: &HeaderMap<HeaderValue>, check_proxy: bool) -> bool {
    // HTTP/1.1, HTTP/2, HTTP/3 keeps alive by default
    let mut conn_keep_alive = !matches!(version, Version::HTTP_09 | Version::HTTP_10);

    if check_proxy {
        // Modern browers will send Proxy-Connection instead of Connection
        // for HTTP/1.0 proxies which blindly forward Connection to remote
        //
        // https://tools.ietf.org/html/rfc7230#appendix-A.1.2
        for value in headers.get_all("Proxy-Connection") {
            if let Ok(value) = value.to_str() {
                if value.eq_ignore_ascii_case("close") {
                    conn_keep_alive = false;
                } else {
                    for part in value.split(',') {
                        let part = part.trim();
                        if part.eq_ignore_ascii_case("keep-alive") {
                            conn_keep_alive = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Connection will replace Proxy-Connection
    //
    // But why client sent both Connection and Proxy-Connection? That's not standard!
    for value in headers.get_all("Connection") {
        if let Ok(value) = value.to_str() {
            if value.eq_ignore_ascii_case("close") {
                conn_keep_alive = false;
            } else {
                for part in value.split(',') {
                    let part = part.trim();

                    if part.eq_ignore_ascii_case("keep-alive") {
                        conn_keep_alive = true;
                        break;
                    }
                }
            }
        }
    }

    conn_keep_alive
}

fn clear_hop_headers(headers: &mut HeaderMap<HeaderValue>) {
    // Clear headers indicated by Connection and Proxy-Connection
    let mut extra_headers = Vec::new();

    for connection in headers.get_all("Connection") {
        if let Ok(conn) = connection.to_str() {
            // close is a command instead of a header
            if conn.eq_ignore_ascii_case("close") {
                continue;
            }

            for header in conn.split(',') {
                let header = header.trim();
                extra_headers.push(header.to_owned());
            }
        }
    }

    for connection in headers.get_all("Proxy-Connection") {
        if let Ok(conn) = connection.to_str() {
            // close is a command instead of a header
            if conn.eq_ignore_ascii_case("close") {
                continue;
            }

            for header in conn.split(',') {
                let header = header.trim();
                extra_headers.push(header.to_owned());
            }
        }
    }

    for header in extra_headers {
        while let Some(..) = headers.remove(&header) {}
    }

    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Connection
    const HOP_BY_HOP_HEADERS: [&str; 9] = [
        "Keep-Alive",
        "Transfer-Encoding",
        "TE",
        "Connection",
        "Trailer",
        "Upgrade",
        "Proxy-Authorization",
        "Proxy-Authenticate",
        "Proxy-Connection", // Not standard, but many implementations do send this header
    ];

    for header in &HOP_BY_HOP_HEADERS {
        while let Some(..) = headers.remove(*header) {}
    }
}

fn set_conn_keep_alive(version: Version, headers: &mut HeaderMap<HeaderValue>, keep_alive: bool) {
    match version {
        Version::HTTP_09 | Version::HTTP_10 => {
            // HTTP/1.0 close connection by default
            if keep_alive {
                headers.insert("Connection", HeaderValue::from_static("keep-alive"));
            }
        }
        _ => {
            // HTTP/1.1, HTTP/2, HTTP/3 keep-alive connection by default
            if !keep_alive {
                headers.insert("Connection", HeaderValue::from_static("close"));
            }
        }
    }
}

fn get_addr_from_header(req: &mut Request<Body>) -> Result<Address, ()> {
    // Try to be compatible as a transparent HTTP proxy
    match req.headers().get("Host") {
        Some(hhost) => match hhost.to_str() {
            Ok(shost) => {
                match Authority::from_str(shost) {
                    Ok(authority) => match authority_addr(req.uri().scheme_str(), &authority) {
                        Some(host) => {
                            trace!("HTTP {} URI {} got host from header: {}", req.method(), req.uri(), host);

                            // Reassemble URI
                            let mut parts = req.uri().clone().into_parts();
                            if parts.scheme.is_none() {
                                // Use http as default.
                                parts.scheme = Some(Scheme::HTTP);
                            }
                            parts.authority = Some(authority);

                            // Replaces URI
                            *req.uri_mut() = Uri::from_parts(parts).expect("Reassemble URI failed");

                            debug!("reassembled URI from \"Host\", {}", req.uri());

                            Ok(host)
                        }
                        None => {
                            error!(
                                "HTTP {} URI {} \"Host\" header invalid, value: {}",
                                req.method(),
                                req.uri(),
                                shost
                            );

                            Err(())
                        }
                    },
                    Err(..) => {
                        error!(
                            "HTTP {} URI {} \"Host\" header is not an Authority, value: {:?}",
                            req.method(),
                            req.uri(),
                            hhost
                        );

                        Err(())
                    }
                }
            }
            Err(..) => {
                error!(
                    "HTTP {} URI {} \"Host\" header invalid encoding, value: {:?}",
                    req.method(),
                    req.uri(),
                    hhost
                );

                Err(())
            }
        },
        None => {
            error!(
                "HTTP {} URI doesn't have valid host and missing the \"Host\" header, URI: {}",
                req.method(),
                req.uri()
            );

            Err(())
        }
    }
}
