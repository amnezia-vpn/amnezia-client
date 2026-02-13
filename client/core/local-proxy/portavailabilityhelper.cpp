#include "portavailabilityhelper.h"

#include <QHostAddress>
#include <QTcpServer>

namespace {
constexpr int kProxyPortMin = 1024;
constexpr int kProxyPortMax = 65535;
}

bool PortAvailabilityHelper::isPortAvailable(int port)
{
    if (port < kProxyPortMin || port > kProxyPortMax) {
        return false;
    }

    QTcpServer server;
    const bool success = server.listen(QHostAddress::LocalHost, static_cast<quint16>(port));
    server.close();
    return success;
}

std::optional<int> PortAvailabilityHelper::findFirstAvailablePort(int startPort, int endPort)
{
    if (startPort < kProxyPortMin) {
        startPort = kProxyPortMin;
    }
    if (endPort > kProxyPortMax) {
        endPort = kProxyPortMax;
    }
    if (startPort > endPort) {
        return std::nullopt;
    }

    for (int port = startPort; port <= endPort; ++port) {
        if (isPortAvailable(port)) {
            return port;
        }
    }

    return std::nullopt;
}

