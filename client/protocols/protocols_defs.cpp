#include "protocols_defs.h"

using namespace amnezia;

QDebug operator<<(QDebug debug, const amnezia::ProtocolEnumNS::Protocol &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << ProtocolProps::protoToString(p);

    return debug;
}

amnezia::Protocol ProtocolProps::protoFromString(QString proto){
    QMetaEnum metaEnum = QMetaEnum::fromType<Protocol>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        Protocol p = static_cast<Protocol>(i);
        if (proto == protoToString(p)) return p;
    }
    return Protocol::Any;
}

QString ProtocolProps::protoToString(amnezia::Protocol p){
    if (p == Protocol::Any) return "";

    QMetaEnum metaEnum = QMetaEnum::fromType<Protocol>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(p));
    return protoKey.toLower();
}

QList<amnezia::Protocol> ProtocolProps::allProtocols()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Protocol>();
    QList<Protocol> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<Protocol>(i));
    }

    return all;
}

TransportProto ProtocolProps::transportProtoFromString(QString p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        TransportProto tp = static_cast<TransportProto>(i);
        if (p.toLower() == transportProtoToString(tp)) return tp;
    }
    return TransportProto::Udp;
}

QString ProtocolProps::transportProtoToString(TransportProto proto, Protocol p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(proto));
    if (p == Protocol::OpenVpn){
        return protoKey.toUpper();
    }
    else {
        return protoKey.toLower();
    }
}


QMap<amnezia::Protocol, QString> ProtocolProps::protocolHumanNames()
{
    return {
        {Protocol::OpenVpn, "OpenVPN"},
        {Protocol::ShadowSocks, "ShadowSocks"},
        {Protocol::Cloak, "Cloak"},
        {Protocol::WireGuard, "WireGuard"},
        {Protocol::TorWebSite, "Web site in TOR network"},
        {Protocol::Dns, "DNS Service"},
        {Protocol::FileShare, "File Sharing Service"},
        {Protocol::Sftp, QObject::tr("Sftp service")}
    };
}

QMap<amnezia::Protocol, QString> ProtocolProps::protocolDescriptions()
{
    return {};
}

amnezia::ServiceType ProtocolProps::protocolService(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return ServiceType::None;
    case Protocol::OpenVpn :      return ServiceType::Vpn;
    case Protocol::Cloak :        return ServiceType::Vpn;
    case Protocol::ShadowSocks :  return ServiceType::Vpn;
    case Protocol::WireGuard :    return ServiceType::Vpn;
    case Protocol::TorWebSite :      return ServiceType::Other;
    case Protocol::Dns :          return ServiceType::Other;
    case Protocol::FileShare :    return ServiceType::Other;
    default:                      return ServiceType::Other;
    }
}

int ProtocolProps::defaultPort(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return -1;
    case Protocol::OpenVpn :      return 1194;
    case Protocol::Cloak :        return 443;
    case Protocol::ShadowSocks :  return 6789;
    case Protocol::WireGuard :    return 51820;
    case Protocol::TorWebSite :      return 443;
    case Protocol::Dns :          return 53;
    case Protocol::FileShare :    return 139;
    case Protocol::Sftp :         return 222;
    default:                      return -1;
    }
}

bool ProtocolProps::defaultPortChangeable(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return false;
    case Protocol::OpenVpn :      return true;
    case Protocol::Cloak :        return true;
    case Protocol::ShadowSocks :  return true;
    case Protocol::WireGuard :    return true;
    case Protocol::TorWebSite :      return true;
    case Protocol::Dns :          return false;
    case Protocol::FileShare :    return false;
    default:                      return -1;
    }
}

TransportProto ProtocolProps::defaultTransportProto(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return TransportProto::Udp;
    case Protocol::OpenVpn :      return TransportProto::Udp;
    case Protocol::Cloak :        return TransportProto::Tcp;
    case Protocol::ShadowSocks :  return TransportProto::Tcp;
    case Protocol::WireGuard :    return TransportProto::Udp;
    // non-vpn
    case Protocol::TorWebSite :   return TransportProto::Tcp;
    case Protocol::Dns :          return TransportProto::Udp;
    case Protocol::FileShare :    return TransportProto::Udp;
    case Protocol::Sftp :         return TransportProto::Tcp;
    default:                      return TransportProto::Udp;
    }
}

bool ProtocolProps::defaultTransportProtoChangeable(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return false;
    case Protocol::OpenVpn :      return true;
    case Protocol::Cloak :        return false;
    case Protocol::ShadowSocks :  return false;
    case Protocol::WireGuard :    return false;
    // non-vpn
    case Protocol::TorWebSite :   return false;
    case Protocol::Dns :          return false;
    case Protocol::FileShare :    return false;
    case Protocol::Sftp :         return false;
    default:                      return false;
    }
}
