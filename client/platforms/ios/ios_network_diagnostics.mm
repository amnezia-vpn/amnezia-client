#include "ios_network_diagnostics.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>

#include <cstdlib>

QString iosDnsServersDiagnostics()
{
    res_state state = static_cast<res_state>(calloc(1, sizeof(struct __res_state)));
    if (!state) {
        return QStringLiteral("failed to allocate resolver state\n");
    }

    QString out;
    if (res_ninit(state) == 0) {
        union res_sockaddr_union addrs[MAXNS];
        const int count = res_getservers(state, addrs, MAXNS);
        for (int i = 0; i < count; ++i) {
            char buf[INET6_ADDRSTRLEN] = {};
            if (addrs[i].sin.sin_family == AF_INET
                && inet_ntop(AF_INET, &addrs[i].sin.sin_addr, buf, sizeof(buf))) {
                out += QString::fromLatin1(buf) + QLatin1Char('\n');
            } else if (addrs[i].sin6.sin6_family == AF_INET6
                       && inet_ntop(AF_INET6, &addrs[i].sin6.sin6_addr, buf, sizeof(buf))) {
                out += QString::fromLatin1(buf) + QLatin1Char('\n');
            }
        }
        res_ndestroy(state);
    }
    free(state);

    if (out.isEmpty()) {
        return QStringLiteral("no DNS servers reported\n");
    }
    return out;
}
