//! Trait of auto-proxy I/O

/// Proxy I/O chooses bypass or proxy automatically
pub trait AutoProxyIo {
    /// Check if the current connection is proxied
    fn is_proxied(&self) -> bool;

    /// Check if the current connection is bypassed
    fn is_bypassed(&self) -> bool {
        !self.is_proxied()
    }
}
