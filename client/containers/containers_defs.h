#ifndef CONTAIERNS_DEFS_H
#define CONTAIERNS_DEFS_H

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
    Q_INVOKABLE static DockerContainer containerFromString(const QString &container);
    Q_INVOKABLE static QString containerToString(DockerContainer container);

    Q_INVOKABLE static QList<DockerContainer> allContainers();

    Q_INVOKABLE static QMap<DockerContainer, QString> containerHumanNames();
    Q_INVOKABLE static QMap<DockerContainer, QString> containerDescriptions();

    // these protocols will be displayed in container settings
    Q_INVOKABLE static QVector<Proto> protocolsForContainer(DockerContainer container);

    Q_INVOKABLE static ServiceType containerService(DockerContainer c);

    // binding between Docker container and main protocol of given container
    // it may be changed fot future containers :)
    Q_INVOKABLE static Proto defaultProtocol(DockerContainer c);

    Q_INVOKABLE static bool isWorkingOnPlatform(DockerContainer c);
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

#endif // CONTAIERNS_DEFS_H
