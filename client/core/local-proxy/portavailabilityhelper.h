#pragma once

#include <optional>

class PortAvailabilityHelper
{
public:
    static bool isPortAvailable(int port);
    static std::optional<int> findFirstAvailablePort(int startPort, int endPort);
};

