//! Proxied HTTP stream

use std::{
    io::{self, ErrorKind},
    pin::Pin,
    task::{self, Poll},
};

use hyper::client::connect::{Connected, Connection};
use pin_project::pin_project;
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use crate::local::net::AutoProxyClientStream;

#[pin_project(project = ProxyHttpStreamProj)]
pub enum ProxyHttpStream {
    Http(#[pin] AutoProxyClientStream),
    #[cfg(feature = "local-http-native-tls")]
    Https(#[pin] tokio_native_tls::TlsStream<AutoProxyClientStream>, bool),
    #[cfg(feature = "local-http-rustls")]
    Https(#[pin] tokio_rustls::client::TlsStream<AutoProxyClientStream>, bool),
}

impl ProxyHttpStream {
    pub fn connect_http(stream: AutoProxyClientStream) -> ProxyHttpStream {
        ProxyHttpStream::Http(stream)
    }

    #[cfg(feature = "local-http-native-tls")]
    pub async fn connect_https(stream: AutoProxyClientStream, domain: &str) -> io::Result<ProxyHttpStream> {
        use native_tls::TlsConnector;

        let cx = match TlsConnector::builder().request_alpns(&["h2", "http/1.1"]).build() {
            Ok(c) => c,
            Err(err) => {
                return Err(io::Error::new(ErrorKind::Other, format!("tls build: {}", err)));
            }
        };
        let cx = tokio_native_tls::TlsConnector::from(cx);

        match cx.connect(domain, stream).await {
            Ok(s) => {
                let negociated_h2 = match s.get_ref().negotiated_alpn() {
                    Ok(Some(alpn)) => alpn == b"h2",
                    Ok(None) => false,
                    Err(err) => {
                        let ierr = io::Error::new(ErrorKind::Other, format!("tls alpn negociate: {}", err));
                        return Err(ierr);
                    }
                };

                Ok(ProxyHttpStream::Https(s, negociated_h2))
            }
            Err(err) => {
                let ierr = io::Error::new(ErrorKind::Other, format!("tls connect: {}", err));
                Err(ierr)
            }
        }
    }

    #[cfg(feature = "local-http-rustls")]
    pub async fn connect_https(stream: AutoProxyClientStream, domain: &str) -> io::Result<ProxyHttpStream> {
        use log::warn;
        use once_cell::sync::Lazy;
        use std::sync::Arc;
        use tokio_rustls::{
            rustls::{ClientConfig, Session},
            webpki::DNSNameRef,
            TlsConnector,
        };

        static TLS_CONFIG: Lazy<Arc<ClientConfig>> = Lazy::new(|| {
            let mut config = ClientConfig::new();

            match rustls_native_certs::load_native_certs() {
                Ok(store) => {
                    config.root_store = store;
                }
                Err((_, err)) => {
                    warn!("failed to load native certs, {}", err);

                    config
                        .root_store
                        .add_server_trust_anchors(&webpki_roots::TLS_SERVER_ROOTS);
                }
            }

            // Try to negociate HTTP/2
            config.alpn_protocols = vec![b"h2".to_vec(), b"http/1.1".to_vec()];
            Arc::new(config)
        });

        let connector = TlsConnector::from(TLS_CONFIG.clone());

        let host = match DNSNameRef::try_from_ascii_str(domain) {
            Ok(n) => n,
            Err(_) => {
                return Err(io::Error::new(
                    ErrorKind::InvalidInput,
                    format!("invalid dnsname \"{}\"", domain),
                ));
            }
        };

        let tls_stream = connector.connect(host, stream).await?;

        let (_, session) = tls_stream.get_ref();
        let negociated_http2 = matches!(session.get_alpn_protocol(), Some(b"h2"));

        Ok(ProxyHttpStream::Https(tls_stream, negociated_http2))
    }

    #[cfg(not(any(feature = "local-http-native-tls", feature = "local-http-rustls")))]
    pub async fn connect_https(_stream: AutoProxyClientStream, _domain: &str) -> io::Result<ProxyHttpStream> {
        let err = io::Error::new(
            ErrorKind::Other,
            "https is not supported, consider enable it by feature \"local-http-native-tls\" or \"local-http-rustls\"",
        );
        Err(err)
    }

    pub fn negociated_http2(&self) -> bool {
        match *self {
            ProxyHttpStream::Http(..) => false,
            #[cfg(any(feature = "local-http-native-tls", feature = "local-http-rustls"))]
            ProxyHttpStream::Https(_, n) => n,
        }
    }
}

macro_rules! forward_call {
    ($self:expr, $method:ident $(, $param:expr)*) => {
        match $self.as_mut().project() {
            ProxyHttpStreamProj::Http(stream) => stream.$method($($param),*),
            #[cfg(any(feature = "local-http-native-tls", feature = "local-http-rustls"))]
            ProxyHttpStreamProj::Https(stream, ..) => stream.$method($($param),*),
        }
    };
}

impl AsyncRead for ProxyHttpStream {
    fn poll_read(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &mut ReadBuf<'_>) -> Poll<io::Result<()>> {
        forward_call!(self, poll_read, cx, buf)
    }
}

impl AsyncWrite for ProxyHttpStream {
    fn poll_write(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        forward_call!(self, poll_write, cx, buf)
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        forward_call!(self, poll_flush, cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut task::Context<'_>) -> Poll<io::Result<()>> {
        forward_call!(self, poll_shutdown, cx)
    }
}

impl Connection for ProxyHttpStream {
    fn connected(&self) -> Connected {
        let conn = Connected::new();
        if self.negociated_http2() {
            conn.negotiated_h2()
        } else {
            conn
        }
    }
}
