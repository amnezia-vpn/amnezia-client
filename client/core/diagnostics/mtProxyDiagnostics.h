#ifndef MTPROXYDIAGNOSTICS_H
#define MTPROXYDIAGNOSTICS_H

#include "containerDiagnostics.h"

#include <QString>

namespace amnezia {
    struct MtProxyDiagnostics : ContainerDiagnostics {
        bool upstreamReachable = false;
        int clientsConnected = -1;
        QString lastConfigRefresh;
        QString statsEndpoint;
    };

} // namespace amnezia

#endif // MTPROXYDIAGNOSTICS_H
