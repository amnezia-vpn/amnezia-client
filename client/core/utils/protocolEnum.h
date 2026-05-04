#ifndef PROTOCOLENUM_H
#define PROTOCOLENUM_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia
{
    namespace ProtocolEnumNS
    {
        Q_NAMESPACE

        enum TransportProto {
            Udp,
            Tcp,
            TcpAndUdp
        };
        Q_ENUM_NS(TransportProto)

        enum Proto {
            Unknown = 0,
            OpenVpn,
            WireGuard,
            Awg,
            Ikev2,
            Xray,
            SSXray,

            // non-vpn
            TorWebSite,
            Dns,
            Sftp,
            Socks5Proxy,
            MtProxy,
        };
        Q_ENUM_NS(Proto)

        enum ServiceType {
            None = 0,
            Vpn,
            Other
        };
        Q_ENUM_NS(ServiceType)
    } // namespace ProtocolEnumNS

    using namespace ProtocolEnumNS;
}

#endif // PROTOCOLENUM_H


