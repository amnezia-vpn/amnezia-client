#ifndef PORTAVAILABILITYHELPER_H
#define PORTAVAILABILITYHELPER_H

#include <optional>

class PortAvailabilityHelper
{
public:
    static bool isPortAvailable(int port);
    static bool waitForPort(int port, int timeoutMs);
    static std::optional<int> findFirstAvailablePort(int startPort, int endPort);
};

#endif // PORTAVAILABILITYHELPER_H
