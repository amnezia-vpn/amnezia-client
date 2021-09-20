#include "containers_defs.h"

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << ContainerProps::containerToString(c);

    return debug;
}

amnezia::DockerContainer ContainerProps::containerFromString(const QString &container){
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        DockerContainer c = static_cast<DockerContainer>(i);
        if (container == containerToString(c)) return c;
    }
    return DockerContainer::None;
}

QString ContainerProps::containerToString(amnezia::DockerContainer c){
    if (c == DockerContainer::None) return "none";

    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));
    return "amnezia-" + containerKey.toLower();
}

QVector<amnezia::Protocol> ContainerProps::protocolsForContainer(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn:
        return { Protocol::OpenVpn };

    case DockerContainer::ShadowSocks:
        return { Protocol::OpenVpn, Protocol::ShadowSocks };

    case DockerContainer::Cloak:
        return { Protocol::OpenVpn, Protocol::ShadowSocks, Protocol::Cloak };

    default:
        return {};
    }
}

QList<DockerContainer> ContainerProps::allContainers()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QList<DockerContainer> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<DockerContainer>(i));
    }

    return all;
}

QMap<DockerContainer, QString> ContainerProps::containerHumanNames()
{
    return {
        {DockerContainer::OpenVpn, "OpenVPN"},
        {DockerContainer::ShadowSocks, "OpenVpn over ShadowSocks"},
        {DockerContainer::Cloak, "OpenVpn over Cloak"},
        {DockerContainer::WireGuard, "WireGuard"},
        {DockerContainer::TorSite, QObject::tr("Web site under TOR")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        {DockerContainer::FileShare, QObject::tr("File Sharing Service")}
    };
}

QMap<DockerContainer, QString> ContainerProps::containerDescriptions()
{
    return {
        {DockerContainer::OpenVpn, QObject::tr("OpenVPN container")},
        {DockerContainer::ShadowSocks, QObject::tr("Container with OpenVpn and ShadowSocks")},
        {DockerContainer::Cloak, QObject::tr("Container with OpenVpn and ShadowSocks protocols "
                                                        "configured with traffic masking by Cloak plugin")},
        {DockerContainer::WireGuard, QObject::tr("WireGuard container")},
        {DockerContainer::TorSite, QObject::tr("Web site under TOR")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        {DockerContainer::FileShare, QObject::tr("File Sharing Service")}

    };
}

amnezia::ServiceType ContainerProps::containerService(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None :         return ServiceType::None;
    case DockerContainer::OpenVpn :      return ServiceType::Vpn;
    case DockerContainer::Cloak :        return ServiceType::Vpn;
    case DockerContainer::ShadowSocks :  return ServiceType::Vpn;
    case DockerContainer::WireGuard :    return ServiceType::Vpn;
    case DockerContainer::TorSite :      return ServiceType::Other;
    case DockerContainer::Dns :          return ServiceType::Other;
    case DockerContainer::FileShare :    return ServiceType::Other;
    default:                             return ServiceType::Other;
    }
}

Protocol ContainerProps::defaultProtocol(DockerContainer c)
{
    return static_cast<Protocol>(c);
}

