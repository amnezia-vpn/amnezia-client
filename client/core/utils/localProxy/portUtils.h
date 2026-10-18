#ifndef PORTUTILS_H
#define PORTUTILS_H

#include <optional>

namespace portUtils
{
    bool isPortAvailable(int port);
    bool waitForPort(int port, int timeoutMs);
    std::optional<int> findFirstAvailablePort(int startPort, int endPort);
}

#endif // PORTUTILS_H
