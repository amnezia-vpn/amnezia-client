//! Signal monitor

#[cfg(unix)]
#[path = "unix.rs"]
mod imp;

#[cfg(windows)]
#[path = "windows.rs"]
mod imp;

#[cfg(not(any(windows, unix)))]
#[path = "other.rs"]
mod imp;

pub use self::imp::create_signal_monitor;
