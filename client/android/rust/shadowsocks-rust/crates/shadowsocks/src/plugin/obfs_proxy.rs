use super::{PluginConfig, PluginMode};
use crate::config::ServerAddr;
use std::{net::SocketAddr, process::Stdio};
use tokio::process::Command;

/// For obfsproxy, we use standalone mode for now.
/// Managed mode needs to use SOCKS5 proxy as forwarder, which is not supported
/// yet.
///
/// The idea of using standalone mode is quite simple, just assemble the
/// internal port into obfsproxy parameters.
///
/// Using manually ran scramblesuit as an example:
/// obfsproxy \
/// --data-dir /tmp/ss_libev_plugin_with_suffix \
/// scramblesuit \
/// --password SOMEMEANINGLESSPASSWORDASEXAMPLE \
/// --dest some.server.org:12345 \
/// client \
/// 127.0.0.1:54321
///
/// In above case, @plugin = "obfsproxy",
/// @plugin_opts = "scramblesuit --password SOMEMEANINGLESSPASSWORDASEXAMPLE"
/// For obfs3, it's even easier, just pass @plugin = "obfsproxy"
/// @plugin_opts = "obfs3"
///
/// And the rest parameters are all assembled here.
/// Some old obfsproxy will not be supported as it doesn't even support
/// "--data-dir" option
pub fn plugin_cmd(plugin: &PluginConfig, remote: &ServerAddr, local: &SocketAddr, mode: PluginMode) -> Command {
    let mut cmd = Command::new(&plugin.plugin);
    cmd.stdin(Stdio::null())
        .kill_on_drop(true)
        .arg("--data-dir")
        .arg(format!("/tmp/{}_{}_{}", plugin.plugin, remote, local)); // FIXME: Not compatible in Windows

    if let Some(ref opt) = plugin.plugin_opts {
        cmd.args(opt.split(' '));
    }

    match mode {
        PluginMode::Client => cmd
            .arg("--dest")
            .arg(remote.to_string())
            .arg("client")
            .arg(local.to_string()),
        PluginMode::Server => cmd
            .arg("--dest")
            .arg(local.to_string())
            .arg("server")
            .arg(remote.to_string()),
    };

    if !plugin.plugin_args.is_empty() {
        cmd.args(&plugin.plugin_args);
    }

    cmd
}
