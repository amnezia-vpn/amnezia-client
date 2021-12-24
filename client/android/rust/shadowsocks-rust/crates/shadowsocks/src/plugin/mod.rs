//! Plugin (SIP003)
//!
//! ```plain
//! +------------+                    +---------------------------+
//! |  SS Client +-- Local Loopback --+  Plugin Client (Tunnel)   +--+
//! +------------+                    +---------------------------+  |
//!                                                                  |
//!             Public Internet (Obfuscated/Transformed traffic) ==> |
//!                                                                  |
//! +------------+                    +---------------------------+  |
//! |  SS Server +-- Local Loopback --+  Plugin Server (Tunnel)   +--+
//! +------------+                    +---------------------------+
//! ```

use std::{
    io,
    net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, TcpListener},
    process::ExitStatus,
    time::{Duration, Instant},
};

use log::{debug, error};
use tokio::{net::TcpStream, process::Child, time};

use crate::config::ServerAddr;

mod obfs_proxy;
mod ss_plugin;

/// Config for plugin
#[derive(Debug, Clone)]
pub struct PluginConfig {
    pub plugin: String,
    pub plugin_opts: Option<String>,
    pub plugin_args: Vec<String>,
}

/// Mode of Plugin
#[derive(Debug, Clone, Copy)]
pub enum PluginMode {
    /// Server's Plugin
    ///
    /// ```plain
    /// LOCAL -> PLUGIN -> SERVER -> REMOTE
    /// ```
    ///
    /// Plugin listens to the inbound address of server
    Server,
    /// Local's Plugin
    ///
    /// ```plain
    /// CLIENT -> LOCAL -> PLUGIN -> SERVER -> ...
    /// ```
    ///
    /// Plugin sends data to the outbound address of server
    Client,
}

/// A shadowsocks SIP004 Plugin
pub struct Plugin {
    process: Child,
    local_addr: SocketAddr,
}

impl Plugin {
    /// Start a plugin subprocess
    ///
    /// `PluginMode::Client`: Plugin listens to `local_addr` and send data to `remote_addr`, client should send data to `local_addr`
    /// `PluginMode::Server`: Plugin listens to `remote_addr` and send data to `local_addr`, server should listen to `local_addr`
    pub fn start(c: &PluginConfig, remote_addr: &ServerAddr, mode: PluginMode) -> io::Result<Plugin> {
        let loop_ip = match remote_addr {
            ServerAddr::SocketAddr(sa) => match sa.ip() {
                IpAddr::V4(..) => Ipv4Addr::LOCALHOST.into(),
                IpAddr::V6(..) => Ipv6Addr::LOCALHOST.into(),
            },
            ServerAddr::DomainName(..) => Ipv4Addr::LOCALHOST.into(),
        };

        let local_addr = get_local_port(loop_ip)?;

        match start_plugin(c, remote_addr, &local_addr, mode) {
            Err(err) => {
                error!(
                    "failed to start plugin \"{}\" for server {}, err: {}",
                    c.plugin, remote_addr, err
                );
                Err(err)
            }
            Ok(process) => {
                match mode {
                    PluginMode::Client => {
                        debug!(
                            "started plugin \"{}\" on {} <-> {} ({})",
                            c.plugin,
                            local_addr,
                            remote_addr,
                            process.id().unwrap_or(0)
                        );
                    }
                    PluginMode::Server => {
                        debug!(
                            "started plugin \"{}\" on {} <-> {} ({})",
                            c.plugin,
                            remote_addr,
                            local_addr,
                            process.id().unwrap_or(0)
                        );
                    }
                }

                Ok(Plugin { process, local_addr })
            }
        }
    }

    /// Join until plugin exits
    pub async fn join(mut self) -> io::Result<ExitStatus> {
        self.process.wait().await
    }

    /// Check if plugin have been started
    pub async fn wait_started(&self, timeout: Duration) -> bool {
        let start_time = Instant::now();

        loop {
            let now_time = Instant::now();
            let elapsed_time = now_time - start_time;
            if elapsed_time >= timeout {
                return false;
            }

            let remain_time = timeout - elapsed_time;
            match time::timeout(remain_time, TcpStream::connect(self.local_addr)).await {
                Ok(Ok(..)) => {
                    return true;
                }
                Ok(Err(..)) => {}
                Err(..) => {
                    return false;
                }
            }
        }
    }

    /// Get listen address of plugin
    pub fn local_addr(&self) -> SocketAddr {
        self.local_addr
    }
}

impl Drop for Plugin {
    // NOTE: Even we have set `Command.kill_on_drop(true)`, processes may not be killed when `Child` handles are dropped.
    // https://github.com/tokio-rs/tokio/issues/2685

    #[cfg(not(unix))]
    fn drop(&mut self) {
        debug!(
            "killing plugin process {:?}, local_addr: {}",
            self.process.id(),
            self.local_addr
        );
        let _ = self.process.start_kill();
    }

    #[cfg(unix)]
    fn drop(&mut self) {
        debug!(
            "terminating plugin process {:?}, local_addr: {}",
            self.process.id(),
            self.local_addr
        );

        let mut terminated = false;

        if let Some(id) = self.process.id() {
            unsafe {
                let ret = libc::kill(id as libc::pid_t, libc::SIGTERM);
                if ret != 0 {
                    let err = io::Error::last_os_error();
                    error!("terminating plugin process {}, error: {}", id, err);
                }
            }

            const MAX_WAIT_DURATION: Duration = Duration::from_millis(10);

            let start_wait = Instant::now();
            loop {
                match self.process.try_wait() {
                    Ok(Some(status)) => {
                        // subprocess is finished
                        debug!(
                            "plugin process {} is terminated gracefully with status: {:?}",
                            id, status
                        );
                        terminated = true;
                        break;
                    }
                    Ok(None) => {}
                    Err(err) => {
                        error!("plugin process waitpid error: {}", err);
                        break;
                    }
                }

                let elapsed = Instant::now() - start_wait;
                if elapsed > MAX_WAIT_DURATION {
                    debug!("plugin process {} isn't terminated in {:?}", id, MAX_WAIT_DURATION);
                    break;
                }

                std::thread::yield_now();
            }
        }

        if !terminated {
            if let Ok(..) = self.process.start_kill() {
                debug!("killed plugin process {:?}", self.process.id());
            }
        }
    }
}

fn start_plugin(plugin: &PluginConfig, remote: &ServerAddr, local: &SocketAddr, mode: PluginMode) -> io::Result<Child> {
    let mut cmd = if plugin.plugin == "obfsproxy" {
        obfs_proxy::plugin_cmd(plugin, remote, local, mode)
    } else {
        ss_plugin::plugin_cmd(plugin, remote, local, mode)
    };
    cmd.spawn()
}

fn get_local_port(loop_ip: IpAddr) -> io::Result<SocketAddr> {
    let listener = TcpListener::bind(SocketAddr::new(loop_ip, 0))?;
    listener.local_addr()
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn generate_random_port() {
        let loop_ip = Ipv4Addr::LOCALHOST.into();
        let addr = get_local_port(loop_ip).unwrap();
        println!("{:?}", addr);
    }
}
