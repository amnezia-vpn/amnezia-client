#include "hysteria2_service.h"
#include "core/networkUtilities.h"

#include <QDebug>
#include <QNetworkInterface>
#include <QCoreApplication>
#include <amnezia_hysteria2.h>

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

bool Hysteria2Service::start(const QString &configYaml)
{
    qDebug() << "Hysteria2Service::start()";

    auto defaultIface = NetworkUtilities::getGatewayAndIface().second;
#ifdef Q_OS_LINUX
    m_defaultIfaceName = defaultIface.name().toUtf8();
#else
    m_defaultIfaceIdx = defaultIface.index();
#endif

    if (auto err = amnezia_hysteria2_setsockcallback(ctxSockCallback, this); err != nullptr) {
        qDebug() << "[hysteria2] sockopt callback failed: " << err;
        amnezia_hysteria2_free(err);
        return false;
    }

    amnezia_hysteria2_setloghandler(ctxLogHandler, this);

    QByteArray bytes = configYaml.toUtf8();
    if (auto err = amnezia_hysteria2_configure(bytes.data()); err != nullptr) {
        qDebug() << "[hysteria2] configuration failed: " << err;
        amnezia_hysteria2_free(err);
        return false;
    }

    if (auto err = amnezia_hysteria2_start(); err != nullptr) {
        qDebug() << "[hysteria2] failed to start: " << err;
        amnezia_hysteria2_free(err);
        return false;
    }

    return true;
}

bool Hysteria2Service::stop()
{
    qDebug() << "Hysteria2Service::stop()";

    if (auto err = amnezia_hysteria2_stop(); err != nullptr) {
        qDebug() << "[hysteria2] failed to stop: " << err;
        amnezia_hysteria2_free(err);
        return false;
    }

    return true;
}

void Hysteria2Service::logHandler(char* str)
{
    QMetaObject::invokeMethod(qApp, [str = QString::fromUtf8(str)] {
        qDebug() << "[hysteria2]" << str;
    }, Qt::QueuedConnection);
}

void Hysteria2Service::sockCallback(uintptr_t fd)
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
        idx = htonl(idx);
        setsockopt(fd, IPPROTO_IP, IP_UNICAST_IF, reinterpret_cast<char *>(&idx), sizeof(idx));
    }
#endif
#ifdef Q_OS_LINUX
    if (!m_defaultIfaceName.isEmpty()) {
        setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, m_defaultIfaceName.data(), m_defaultIfaceName.size());
    }
#endif
}
