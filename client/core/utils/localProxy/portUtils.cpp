#include "portUtils.h"

#include "localProxyConstants.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QThread>

using namespace amnezia;

bool portUtils::isPortAvailable(int port)
{
    if (port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        return false;
    }

    QTcpServer server;
    const bool success = server.listen(QHostAddress::LocalHost, static_cast<quint16>(port));
    server.close();
    return success;
}

bool portUtils::waitForPort(int port, int timeoutMs, const PortCheck &isAvailable)
{
    QElapsedTimer timer;
    timer.start();

    while (!isAvailable(port)) {
        if (timer.elapsed() >= timeoutMs) {
            return false;
        }
        QThread::msleep(200);
    }

    return true;
}

std::optional<int> portUtils::findFirstAvailablePort(int startPort, int endPort, const PortCheck &isAvailable)
{
    startPort = qMax(startPort, localProxy::proxyPortMin);
    endPort = qMin(endPort, localProxy::proxyPortMax);

    for (int port = startPort; port <= endPort; ++port) {
        if (isAvailable(port)) {
            return port;
        }
    }

    return std::nullopt;
}
