#include "protocols_defs.h"

using namespace amnezia;

QDebug operator<<(QDebug debug, const amnezia::ProtocolEnumNS::Proto &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << ProtocolProps::protoToString(p);

    return debug;
}

amnezia::Proto ProtocolProps::protoFromString(QString proto){
    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        Proto p = static_cast<Proto>(i);
        if (proto == protoToString(p)) return p;
    }
    return Proto::Any;
}

QString ProtocolProps::protoToString(amnezia::Proto p){
    if (p == Proto::Any) return "";

    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(p));
    return protoKey.toLower();
}

QList<amnezia::Proto> ProtocolProps::allProtocols()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    QList<Proto> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<Proto>(i));
    }

    return all;
}

TransportProto ProtocolProps::transportProtoFromString(QString p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        TransportProto tp = static_cast<TransportProto>(i);
        if (p.toLower() == transportProtoToString(tp).toLower()) return tp;
    }
    return TransportProto::Udp;
}

QString ProtocolProps::transportProtoToString(TransportProto proto, Proto p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(proto));
    return protoKey.toLower();

//    if (p == Protocol::OpenVpn){
//        return protoKey.toLower();
//    }
//    else if (p == Protocol::ShadowSocks) {
//        return protoKey.toUpper();
//    }
//    else {
//        return protoKey.toLower();
//    }
}


QMap<amnezia::Proto, QString> ProtocolProps::protocolHumanNames()
{
    return {
        {Proto::OpenVpn, "OpenVPN"},
        {Proto::ShadowSocks, "ShadowSocks"},
        {Proto::Cloak, "Cloak"},
        {Proto::WireGuard, "WireGuard"},
        {Proto::Ikev2, "IKEv2"},
        {Proto::L2tp, "L2TP"},

        {Proto::TorWebSite, "Web site in Tor network"},
        {Proto::Dns, "DNS Service"},
        {Proto::FileShare, "File Sharing Service"},
        {Proto::Sftp, QObject::tr("Sftp service")},
        {Proto::Nextcloud, QObject::tr("Nextcloud")}
    };
}

QMap<amnezia::Proto, QString> ProtocolProps::protocolDescriptions()
{
    return {};
}

amnezia::ServiceType ProtocolProps::protocolService(Proto p)
{
    switch (p) {
    case Proto::Any :          return ServiceType::None;
    case Proto::OpenVpn :      return ServiceType::Vpn;
    case Proto::Cloak :        return ServiceType::Vpn;
    case Proto::ShadowSocks :  return ServiceType::Vpn;
    case Proto::WireGuard :    return ServiceType::Vpn;
    case Proto::TorWebSite :      return ServiceType::Other;
    case Proto::Dns :          return ServiceType::Other;
    case Proto::FileShare :    return ServiceType::Other;
    default:                      return ServiceType::Other;
    }
}

int ProtocolProps::defaultPort(Proto p)
{
    switch (p) {
    case Proto::Any :          return -1;
    case Proto::OpenVpn :      return 1194;
    case Proto::Cloak :        return 443;
    case Proto::ShadowSocks :  return 6789;
    case Proto::WireGuard :    return 51820;
    case Proto::Ikev2 :        return -1;
    case Proto::L2tp :         return -1;

    case Proto::TorWebSite :   return -1;
    case Proto::Dns :          return 53;
    case Proto::FileShare :    return 139;
    case Proto::Sftp :         return 222;
    case Proto::Nextcloud :    return 8080;
    default:                      return -1;
    }
}

bool ProtocolProps::defaultPortChangeable(Proto p)
{
    switch (p) {
    case Proto::Any :          return false;
    case Proto::OpenVpn :      return true;
    case Proto::Cloak :        return true;
    case Proto::ShadowSocks :  return true;
    case Proto::WireGuard :    return true;
    case Proto::Ikev2 :        return false;
    case Proto::L2tp :         return false;


    case Proto::TorWebSite :   return true;
    case Proto::Dns :          return false;
    case Proto::FileShare :    return false;
    default:                      return -1;
    }
}

TransportProto ProtocolProps::defaultTransportProto(Proto p)
{
    switch (p) {
    case Proto::Any :          return TransportProto::Udp;
    case Proto::OpenVpn :      return TransportProto::Udp;
    case Proto::Cloak :        return TransportProto::Tcp;
    case Proto::ShadowSocks :  return TransportProto::Tcp;
    case Proto::WireGuard :    return TransportProto::Udp;
    case Proto::Ikev2 :        return TransportProto::Udp;
    case Proto::L2tp :         return TransportProto::Udp;
    // non-vpn
    case Proto::TorWebSite :   return TransportProto::Tcp;
    case Proto::Dns :          return TransportProto::Udp;
    case Proto::FileShare :    return TransportProto::Udp;
    case Proto::Sftp :         return TransportProto::Tcp;
    case Proto::Nextcloud :    return TransportProto::Tcp;
    }
}

bool ProtocolProps::defaultTransportProtoChangeable(Proto p)
{
    switch (p) {
    case Proto::Any :          return false;
    case Proto::OpenVpn :      return true;
    case Proto::Cloak :        return false;
    case Proto::ShadowSocks :  return false;
    case Proto::WireGuard :    return false;
    case Proto::Ikev2 :        return false;
    case Proto::L2tp :         return false;
    // non-vpn
    case Proto::TorWebSite :   return false;
    case Proto::Dns :          return false;
    case Proto::FileShare :    return false;
    case Proto::Sftp :         return false;
    default:                      return false;
    }
}

QString ProtocolProps::key_proto_config_data(Proto p)
{
    return protoToString(p) + "_config_data";
}

QString ProtocolProps::key_proto_config_path(Proto p)
{
    return protoToString(p) + "_config_path";
}
