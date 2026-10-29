#ifndef IPC_H
#define IPC_H

#include <QString>

#define IPC_SERVICE_URL "local:AmneziaVpnIpcInterface"

namespace amnezia {

enum class TunnelStatus {
    Disconnected,
    Connected,
    NoNetwork
};

enum class KillSwitchMode {
    Disabled,
    Soft,
    Hard
};

inline QString getIpcServiceUrl() {
#ifdef Q_OS_WIN
    return IPC_SERVICE_URL;
#else
    return QString("/tmp/%1").arg(IPC_SERVICE_URL);
#endif
}

} // namespace amnezia

#endif // IPC_H
