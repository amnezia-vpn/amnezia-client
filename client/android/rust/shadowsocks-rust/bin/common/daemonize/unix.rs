use std::path::Path;

use daemonize::Daemonize;
use log::error;

/// Daemonize a server process in a *nix standard way
///
/// This function will redirect `stdout`, `stderr` to `/dev/null`,
/// and follow the exact behavior in shadowsocks-libev
pub fn daemonize<F: AsRef<Path>>(pid_path: Option<F>) {
    let mut d = Daemonize::new().umask(0).chroot("/");
    if let Some(p) = pid_path {
        d = d.pid_file(p);
    }

    if let Err(err) = d.start() {
        error!("failed to daemonize, {}", err);
    }
}
