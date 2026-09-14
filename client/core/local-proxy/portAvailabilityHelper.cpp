#include "portAvailabilityHelper.h"

#include "localProxyDefs.h"

#include <QHostAddress>
#include <QTcpServer>

using namespace amnezia;

bool PortAvailabilityHelper::isPortAvailable(int port)
{
    if (port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        return false;
    }

    QTcpServer server;
    const bool success = server.listen(QHostAddress::LocalHost, static_cast<quint16>(port));
    server.close();
    return success;
}

std::optional<int> PortAvailabilityHelper::findFirstAvailablePort(int startPort, int endPort)
{
    startPort = qMax(startPort, localProxy::proxyPortMin);
    endPort = qMin(endPort, localProxy::proxyPortMax);

    for (int port = startPort; port <= endPort; ++port) {
        if (isPortAvailable(port)) {
            return port;
        }
    }

    return std::nullopt;
}
