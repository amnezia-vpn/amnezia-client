#ifndef TELEMTDIAGNOSTICS_H
#define TELEMTDIAGNOSTICS_H

#include "containerDiagnostics.h"

#include <QString>

namespace amnezia
{
    struct TelemtDiagnostics : ContainerDiagnostics
    {
        bool upstreamReachable = false;
        int clientsConnected = -1;
        QString lastConfigRefresh;
        QString statsEndpoint;
    };

} // namespace amnezia

#endif // TELEMTDIAGNOSTICS_H
