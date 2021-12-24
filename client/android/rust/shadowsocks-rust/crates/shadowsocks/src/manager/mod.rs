//! Shadowsocks Server manager
//!
//! Service for managing multiple relay servers. [Manage Multiple Users](https://github.com/shadowsocks/shadowsocks/wiki/Manage-Multiple-Users)

pub use self::{client::ManagerClient, listener::ManagerListener};

pub mod client;
pub mod datagram;
pub mod error;
pub mod listener;
pub mod protocol;
