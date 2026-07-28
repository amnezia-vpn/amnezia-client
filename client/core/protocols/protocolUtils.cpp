#include "protocolUtils.h"

#include <QRandomGenerator>
#include <QJsonObject>
#include <QObject>

using namespace amnezia;

QList<Proto> ProtocolUtils::allProtocols()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    QList<Proto> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<Proto>(i));
    }

    return all;
}

TransportProto ProtocolUtils::transportProtoFromString(QString p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        TransportProto tp = static_cast<TransportProto>(i);
        if (p.toLower() == transportProtoToString(tp).toLower())
            return tp;
    }
    return TransportProto::Udp;
}

QString ProtocolUtils::transportProtoToString(TransportProto proto, Proto p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(proto));
    return protoKey.toLower();
}

Proto ProtocolUtils::protoFromString(QString proto)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        Proto p = static_cast<Proto>(i);
        if (proto == protoToString(p))
            return p;
    }
    return Proto::Unknown;
}

QString ProtocolUtils::protoToString(Proto p)
{
    if (p == Proto::Unknown)
        return "";

    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(p));
    return protoKey.toLower();
}

QMap<Proto, QString> ProtocolUtils::protocolHumanNames()
{
    return { { Proto::OpenVpn, "OpenVPN" },
             { Proto::WireGuard, "WireGuard" },
             { Proto::Awg, "AmneziaWG" },
             { Proto::Ikev2, "IKEv2" },
             { Proto::Xray, "XRay" },
             { Proto::SSXray, "Shadowsocks"},

             { Proto::TorWebSite, "Website in Tor network" },
             { Proto::Dns, "DNS Service" },
             { Proto::Sftp, QObject::tr("SFTP service") },
             { Proto::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
             { Proto::MtProxy, QObject::tr("MTProxy (Telegram)") },
             { Proto::Telemt, QObject::tr("Telemt (Telegram)") },
    };
}

QMap<Proto, QString> ProtocolUtils::protocolDescriptions()
{
    return {};
}

ServiceType ProtocolUtils::protocolService(Proto p)
{
    switch (p) {
    case Proto::Unknown: return ServiceType::None;
    case Proto::SSXray: return ServiceType::None;

    case Proto::OpenVpn: return ServiceType::Vpn;
    case Proto::WireGuard: return ServiceType::Vpn;
    case Proto::Awg: return ServiceType::Vpn;
    case Proto::Ikev2: return ServiceType::Vpn;
    case Proto::Xray: return ServiceType::Vpn;

    case Proto::TorWebSite: return ServiceType::Other;
    case Proto::Dns: return ServiceType::Other;
    case Proto::Sftp: return ServiceType::Other;
    case Proto::Socks5Proxy: return ServiceType::Other;
    case Proto::MtProxy: return ServiceType::Other;
    case Proto::Telemt: return ServiceType::Other;
    default: return ServiceType::Other;
    }
}

int ProtocolUtils::getPortForInstall(Proto p)
{
    switch (p) {
    case Awg:
    case WireGuard:
    case OpenVpn:
    case Socks5Proxy:
        return QRandomGenerator::global()->bounded(30000, 50000);
    case MtProxy:
    case Telemt:
    default:
        return defaultPort(p);
    }
}

int ProtocolUtils::defaultPort(Proto p)
{
    switch (p) {
    case Proto::Unknown: return -1;
    case Proto::OpenVpn: return QString(protocols::openvpn::defaultPort).toInt();
    case Proto::WireGuard: return QString(protocols::wireguard::defaultPort).toInt();
    case Proto::Awg: return QString(protocols::awg::defaultPort).toInt();
    case Proto::Xray: return QString(protocols::xray::defaultPort).toInt();
    case Proto::Ikev2: return -1;

    case Proto::TorWebSite: return -1;
    case Proto::Dns: return 53;
    case Proto::Sftp: return 222;
    case Proto::Socks5Proxy: return 38080;
    case Proto::MtProxy: return QString(protocols::mtProxy::defaultPort).toInt();
    case Proto::Telemt: return QString(protocols::telemt::defaultPort).toInt();
    default: return -1;
    }
}

bool ProtocolUtils::defaultPortChangeable(Proto p)
{
    switch (p) {
    case Proto::Unknown: return false;
    case Proto::OpenVpn: return true;
    case Proto::WireGuard: return true;
    case Proto::Awg: return true;
    case Proto::Ikev2: return false;
    case Proto::Xray: return true;

    case Proto::TorWebSite: return false;
    case Proto::Dns: return false;
    case Proto::Sftp: return true;
    case Proto::Socks5Proxy: return true;
    case Proto::MtProxy: return true;
    case Proto::Telemt: return true;
    default: return false;
    }
}

TransportProto ProtocolUtils::defaultTransportProto(Proto p)
{
    switch (p) {
    case Proto::Unknown: return TransportProto::Udp;
    case Proto::OpenVpn: return TransportProto::Udp;
    case Proto::WireGuard: return TransportProto::Udp;
    case Proto::Awg: return TransportProto::Udp;
    case Proto::Ikev2: return TransportProto::Udp;
    case Proto::Xray: return TransportProto::Tcp;
    case Proto::SSXray: return TransportProto::Tcp;

    // non-vpn
    case Proto::TorWebSite: return TransportProto::Tcp;
    case Proto::Dns: return TransportProto::Udp;
    case Proto::Sftp: return TransportProto::Tcp;
    case Proto::Socks5Proxy: return TransportProto::Tcp;
    case Proto::MtProxy: return TransportProto::Tcp;
    case Proto::Telemt: return TransportProto::Tcp;
    default: return TransportProto::Udp;
    }
}

bool ProtocolUtils::defaultTransportProtoChangeable(Proto p)
{
    switch (p) {
    case Proto::Unknown: return false;
    case Proto::OpenVpn: return true;
    case Proto::WireGuard: return false;
    case Proto::Awg: return false;
    case Proto::Ikev2: return false;
    case Proto::Xray: return false;

    // non-vpn
    case Proto::TorWebSite: return false;
    case Proto::Dns: return false;
    case Proto::Sftp: return false;
    case Proto::Socks5Proxy: return false;
    case Proto::MtProxy: return false;
    case Proto::Telemt: return false;
    default: return false;
    }
}

QString ProtocolUtils::key_proto_config_data(Proto p)
{
    return protoToString(p) + "_config_data";
}

QString ProtocolUtils::key_proto_config_path(Proto p)
{
    return protoToString(p) + "_config_path";
}

QString ProtocolUtils::getProtocolVersion(const QJsonObject &protocolConfig)
{
    return protocolConfig.value(configKey::protocolVersion).toString();
}

QString ProtocolUtils::getProtocolVersionString(const QJsonObject &protocolConfig)
{
    auto version = getProtocolVersion(protocolConfig);

    if (version == protocols::awg::awgV2) return QObject::tr(" (version 2)");
    if (version == protocols::awg::awgV1_5) return QObject::tr(" (version 1.5)");
    return "";
}
