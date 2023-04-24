#ifndef CONTAINERS_DEFS_H
#define CONTAINERS_DEFS_H

#include <QObject>
#include <QQmlEngine>

#include "../protocols/protocols_defs.h"

using namespace amnezia;

namespace amnezia {

namespace ContainerEnumNS {
Q_NAMESPACE
enum DockerContainer {
    None = 0,
    OpenVpn,
    ShadowSocks,
    Cloak,
    WireGuard,
    Ipsec,

    //non-vpn
    TorWebSite,
    Dns,
    //FileShare,
    Sftp
};
Q_ENUM_NS(DockerContainer)
} // namespace ContainerEnumNS

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;

class ContainerProps : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static amnezia::DockerContainer containerFromString(const QString &container);
    Q_INVOKABLE static QString containerToString(amnezia::DockerContainer container);
    Q_INVOKABLE static QString containerTypeToString(amnezia::DockerContainer c);

    Q_INVOKABLE static QList<amnezia::DockerContainer> allContainers();

    Q_INVOKABLE static QMap<amnezia::DockerContainer, QString> containerHumanNames();
    Q_INVOKABLE static QMap<amnezia::DockerContainer, QString> containerDescriptions();

    // these protocols will be displayed in container settings
    Q_INVOKABLE static QVector<amnezia::Proto> protocolsForContainer(amnezia::DockerContainer container);

    Q_INVOKABLE static amnezia::ServiceType containerService(amnezia::DockerContainer c);

    // binding between Docker container and main protocol of given container
    // it may be changed fot future containers :)
    Q_INVOKABLE static amnezia::Proto defaultProtocol(amnezia::DockerContainer c);

    Q_INVOKABLE static bool isSupportedByCurrentPlatform(amnezia::DockerContainer c);
    Q_INVOKABLE static QStringList fixedPortsForContainer(amnezia::DockerContainer c);
};



static void declareQmlContainerEnum() {   
    qmlRegisterUncreatableMetaObject(
                ContainerEnumNS::staticMetaObject,
                "ContainerEnum",
                1, 0,
                "ContainerEnum",
                "Error: only enums"
                );
}

} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c);

#endif // CONTAINERS_DEFS_H
