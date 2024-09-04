#ifndef IPC_H
#define IPC_H

#include <QObject>
#include <QString>

#include "../client/utilities.h"

#define IPC_SERVICE_URL "local:AmneziaVpnIpcInterface"

namespace amnezia {

enum PermittedProcess {
    OpenVPN,
    Wireguard,
    Tun2Socks,
    CertUtil,
    GoodbyeDPI
};

inline QString permittedProcessPath(PermittedProcess pid)
{
    if (pid == PermittedProcess::OpenVPN) {
        return Utils::openVpnExecPath();
    } else if (pid == PermittedProcess::Wireguard) {
        return Utils::wireguardExecPath();
    } else if (pid == PermittedProcess::CertUtil) {
        return Utils::certUtilPath();
    } else if (pid == PermittedProcess::Tun2Socks) {
        return Utils::tun2socksPath();
    } else if (pid == PermittedProcess::GoodbyeDPI){
        return Utils::goodbyedpiPath();
    }
    return "";
}


inline QString getIpcServiceUrl() {
#ifdef Q_OS_WIN
    return IPC_SERVICE_URL;
#else
    return QString("/tmp/%1").arg(IPC_SERVICE_URL);
#endif
}

inline QString getIpcProcessUrl(int pid) {
#ifdef Q_OS_WIN
    return QString("%1_%2").arg(IPC_SERVICE_URL).arg(pid);
#else
    return QString("/tmp/%1_%2").arg(IPC_SERVICE_URL).arg(pid);
#endif
}


} // namespace amnezia

#endif // IPC_H
