#ifndef IPC_H
#define IPC_H

#include <QString>

#define IPC_SERVICE_URL "local:AmneziaVpnIpcInterface"

namespace amnezia {
inline QString getIpcProcessUrl(int pid) { return QString("%1_%2").arg(IPC_SERVICE_URL).arg(pid); }
}

#endif // IPC_H
