#pragma once
#include <QString>

enum class DaemonError {
    None,
    SwitchServer,
    DnsUtils,
    UpdateResolvers,
    RunSwitch,
    ParseConfig,
    Reconfig,
    AddInterface,
    AddInterfaceIps,
    UpdatePeer,
    SetMTU,
    UpdateRoutePrefix,
    Run,
    SplitTunnel
};
static QString localizeDaemonError(DaemonError err) {
    switch (err) {
        case DaemonError::None:
            return "DaemonError::None";
        case DaemonError::SwitchServer:
            return "DaemonError::SwitchServer";
        case DaemonError::DnsUtils:
            return "DaemonError::DnsUtils";
        case DaemonError::UpdateResolvers:
            return "DaemonError::UpdateResolvers";
        case DaemonError::RunSwitch:
            return "DaemonError::RunSwitch";
        case DaemonError::ParseConfig:
            return "DaemonError::ParseConfig";
        case DaemonError::Reconfig:
            return "DaemonError::Reconfig";
        case DaemonError::AddInterface:
            return "DaemonError::AddInterface";
        case DaemonError::AddInterfaceIps:
            return "DaemonError::AddInterfaceIps";
        case DaemonError::UpdatePeer:
            return "DaemonError::UpdatePeer";
        case DaemonError::SetMTU:
            return "DaemonError::SetMTU";
        case DaemonError::UpdateRoutePrefix:
            return "DaemonError::UpdateRoutePrefix";
        case DaemonError::SplitTunnel:
            return "DaemonError::SplitTunnel";
    }
    return "Unknown";
}
