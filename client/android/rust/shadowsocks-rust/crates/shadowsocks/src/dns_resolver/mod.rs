//! Asynchronous DNS resolver
#![macro_use]

pub use self::resolver::{DnsResolve, DnsResolver};

mod resolver;
#[cfg(feature = "trust-dns")]
mod trust_dns_resolver;

/// Helper macro for resolving host and then process each addresses
#[macro_export]
macro_rules! lookup_then {
    ($context:expr, $addr:expr, $port:expr, |$resolved_addr:ident| $body:block) => {{
        let mut result = None;

        for $resolved_addr in $context.dns_resolve($addr, $port).await? {
            match $body {
                Ok(r) => {
                    result = Some(Ok(($resolved_addr, r)));
                    break;
                }
                Err(err) => {
                    result = Some(Err(err));
                }
            }
        }

        result.expect("resolved empty address")
    }};
}
