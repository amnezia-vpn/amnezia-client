#include "xray.h"
#include "core/networkUtilities.h"

#include <QDebug>
#include <QNetworkInterface>
#include <QCoreApplication>
#include <amnezia_xray.h>
#include <qdebug.h>

#ifdef Q_OS_DARWIN
    #include <arpa/inet.h>
    #include <cerrno>
    #include <cstddef>
    #include <cstdint>
    #include <cstring>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <sys/socket.h>
#endif
#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif
#ifdef Q_OS_LINUX
    #include <sys/socket.h>
#endif

void Xray::startXray(const QString &cfg)
{
    qDebug() << "Xray::startXray()";

    auto defaultIface = NetworkUtilities::getGatewayAndIface().second;
#ifdef Q_OS_LINUX
    m_defaultIfaceName = defaultIface.name().toUtf8();
#else
    m_defaultIfaceIdx = defaultIface.index();
#endif

    if (auto err = amnezia_xray_setsockcallback(ctxSockCallback, this); err != nullptr) {
        qDebug() << "[xray] sockopt failed: " << err;
        free(err);
        return;
    }

    QByteArray bytes = cfg.toUtf8();
    if (auto err = amnezia_xray_configure(bytes.data()); err != nullptr) {
        qDebug() << "[xray] configuration failed: " << err;
        free(err);
        return;
    }

    amnezia_xray_setloghandler(ctxLogHandler, this);

    if (auto err = amnezia_xray_start(); err != nullptr) {
        qDebug() << "[xray] failed to start: " << err;
        free(err);
        return;
    }
}

void Xray::stopXray()
{
    qDebug() << "Xray::stopXray()";
    if (auto err = amnezia_xray_stop(); err != nullptr) {
        qDebug() << "[xray] failed to stop: " << err;
        free(err);
        return;
    }
}

void Xray::logHandler(char* str)
{
    QMetaObject::invokeMethod(qApp, [str = QString::fromUtf8(str)] {
        qDebug() << "[xray]" << str;
    }, Qt::QueuedConnection);
}

void Xray::sockCallback(uintptr_t fd)
{
#ifdef Q_OS_MAC
    if (m_defaultIfaceIdx > 0) {
        setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &m_defaultIfaceIdx, sizeof(m_defaultIfaceIdx));
        setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &m_defaultIfaceIdx, sizeof(m_defaultIfaceIdx));
    }
#endif
#ifdef Q_OS_WIN
    if (DWORD idx = m_defaultIfaceIdx; idx > 0) {
        setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_IF, reinterpret_cast<char *>(&idx), sizeof(idx));
        idx = htonl(idx); // IP_UNICAST_IF expects index in network byte order
        setsockopt(fd, IPPROTO_IP, IP_UNICAST_IF, reinterpret_cast<char *>(&idx), sizeof(idx));
    }
#endif
#ifdef Q_OS_LINUX
    if (!m_defaultIfaceName.isEmpty()) {
        setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, m_defaultIfaceName.data(), m_defaultIfaceName.size());
    }
#endif
}
