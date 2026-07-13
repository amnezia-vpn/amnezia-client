#ifndef CONTAINERENUM_H
#define CONTAINERENUM_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia
{
    namespace ContainerEnumNS
    {
        Q_NAMESPACE
        enum DockerContainer {
            None = 0,
            Awg,
            Awg2,
            WireGuard,
            OpenVpn,
            Cloak,
            ShadowSocks,
            Ipsec,
            Xray,
            SSXray,

            // non-vpn
            TorWebSite,
            Dns,
            Sftp,
            Socks5Proxy,
            MtProxy,
            Telemt,
        };
        Q_ENUM_NS(DockerContainer)
    } // namespace ContainerEnumNS

    using namespace ContainerEnumNS;
}

#endif // CONTAINERENUM_H


