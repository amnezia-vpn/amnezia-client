//! Server flow statistic

use std::sync::atomic::Ordering;

#[cfg(not(any(target_arch = "mips", target_arch = "powerpc")))]
type FlowCounter = std::sync::atomic::AtomicU64;
#[cfg(any(target_arch = "mips", target_arch = "powerpc"))]
type FlowCounter = std::sync::atomic::AtomicU32;

/// Connection flow statistic
pub struct FlowStat {
    tx: FlowCounter,
    rx: FlowCounter,
}

impl FlowStat {
    /// Create an empty flow statistic
    pub fn new() -> FlowStat {
        FlowStat {
            tx: FlowCounter::new(0),
            rx: FlowCounter::new(0),
        }
    }

    /// Transmitted bytes count
    #[cfg(not(any(target_arch = "mips", target_arch = "powerpc")))]
    pub fn tx(&self) -> u64 {
        self.tx.load(Ordering::Relaxed)
    }

    /// Transmitted bytes count
    #[cfg(any(target_arch = "mips", target_arch = "powerpc"))]
    pub fn tx(&self) -> u64 {
        self.tx.load(Ordering::Relaxed) as u64
    }

    /// Increase transmitted bytes
    #[cfg(not(any(target_arch = "mips", target_arch = "powerpc")))]
    pub fn incr_tx(&self, n: u64) {
        self.tx.fetch_add(n, Ordering::AcqRel);
    }

    /// Increase transmitted bytes
    #[cfg(any(target_arch = "mips", target_arch = "powerpc"))]
    pub fn incr_tx(&self, n: u64) {
        self.tx.fetch_add(n as u32, Ordering::AcqRel);
    }

    /// Received bytes count
    #[cfg(not(any(target_arch = "mips", target_arch = "powerpc")))]
    pub fn rx(&self) -> u64 {
        self.rx.load(Ordering::Relaxed)
    }

    /// Received bytes count
    #[cfg(any(target_arch = "mips", target_arch = "powerpc"))]
    pub fn rx(&self) -> u64 {
        self.rx.load(Ordering::Relaxed) as u64
    }

    /// Increase received bytes
    #[cfg(not(any(target_arch = "mips", target_arch = "powerpc")))]
    pub fn incr_rx(&self, n: u64) {
        self.rx.fetch_add(n, Ordering::AcqRel);
    }

    /// Increase received bytes
    #[cfg(any(target_arch = "mips", target_arch = "powerpc"))]
    pub fn incr_rx(&self, n: u64) {
        self.rx.fetch_add(n as u32, Ordering::AcqRel);
    }
}
