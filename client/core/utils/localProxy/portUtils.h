#ifndef PORTUTILS_H
#define PORTUTILS_H

#include <functional>
#include <optional>

namespace portUtils
{
    using PortCheck = std::function<bool(int)>;

    bool isPortAvailable(int port);
    bool waitForPort(int port, int timeoutMs, const PortCheck &isAvailable = isPortAvailable);
    std::optional<int> findFirstAvailablePort(int startPort, int endPort, const PortCheck &isAvailable = isPortAvailable);
}

#endif // PORTUTILS_H
