use super::{PluginConfig, PluginMode};
use crate::config::ServerAddr;
use log::trace;
use std::{net::SocketAddr, process::Stdio};
use tokio::process::Command;

pub fn plugin_cmd(plugin: &PluginConfig, remote: &ServerAddr, local: &SocketAddr, _mode: PluginMode) -> Command {
    trace!(
        "Starting plugin \"{}\", opt: {:?}, arg: {:?}, remote: {}, local: {}",
        plugin.plugin,
        plugin.plugin_opts,
        plugin.plugin_args,
        remote,
        local
    );

    let mut cmd = Command::new(&plugin.plugin);
    cmd.env("SS_REMOTE_HOST", remote.host())
        .env("SS_REMOTE_PORT", remote.port().to_string())
        .env("SS_LOCAL_HOST", local.ip().to_string())
        .env("SS_LOCAL_PORT", local.port().to_string())
        .stdin(Stdio::null())
        .kill_on_drop(true);

    if let Some(ref opt) = plugin.plugin_opts {
        cmd.env("SS_PLUGIN_OPTIONS", opt);
    }

    if !plugin.plugin_args.is_empty() {
        cmd.args(&plugin.plugin_args);
    }

    cmd
}
