#ifndef LOCALPROXYDEFS_H
#define LOCALPROXYDEFS_H

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcLocalProxy)

namespace amnezia::localProxy
{
    constexpr int defaultProxyPort = 10808;
    constexpr int proxyPortMin = 1024;
    constexpr int proxyPortMax = 65535;
    constexpr quint16 apiPort = 49490;
}

#endif // LOCALPROXYDEFS_H
